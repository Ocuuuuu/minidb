#include "../include/storage/BufferManager.h"
#include <iostream>

namespace minidb {
namespace storage {

BufferManager::BufferManager(std::shared_ptr<DiskManager> disk_manager,
                           size_t pool_size,
                           BufferReplacementPolicy policy)
    : disk_manager_(disk_manager), pool_size_(pool_size), policy_(policy) {}

BufferManager::~BufferManager() {
    flushAllPages();
}

Page* BufferManager::fetchPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    // 检查页面是否已在缓冲池中
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        // 页面在缓冲池中，更新访问时间
        hit_count_++;
        updateAccessTime(page_id);
        return &(it->second.page);
    }

    // 页面不在缓冲池中
    miss_count_++;

    // 如果缓冲池已满，需要淘汰一个页面
    if (page_table_.size() >= pool_size_) {
        if (!evictPage()) {
            throw BufferPoolFullException("Buffer pool is full and cannot evict any page");
        }
    }

    // 从磁盘读取页面
    char page_data[PAGE_SIZE];
    disk_manager_->readPage(page_id, page_data);

    // 创建新的缓冲帧
    BufferFrame frame;
    frame.page.deserialize(page_data);
    frame.pin_count = 0;
    frame.is_dirty = false;

    // 添加到LRU列表和页面表
    lru_list_.push_front(page_id);
    frame.iterator = lru_list_.begin();

    auto result = page_table_.emplace(page_id, std::move(frame));
    if (!result.second) {
        throw BufferPoolFullException("Failed to insert page into buffer pool");
    }

    return &(result.first->second.page);
}

void BufferManager::pinPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        throw BufferPoolFullException("Page not in buffer pool: " + std::to_string(page_id));
    }

    it->second.pin_count++;
}

void BufferManager::unpinPage(PageID page_id, bool is_dirty) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        throw BufferPoolFullException("Page not in buffer pool: " + std::to_string(page_id));
    }

    if (it->second.pin_count == 0) {
        throw BufferPoolFullException("Page pin count already zero: " + std::to_string(page_id));
    }

    it->second.pin_count--;
    it->second.is_dirty = it->second.is_dirty || is_dirty;

    // 如果页面不再被固定，更新访问时间
    if (it->second.pin_count == 0) {
        updateAccessTime(page_id);
    }
}

void BufferManager::flushPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return; // 页面不在缓冲池中，无需刷新
    }

    if (it->second.is_dirty) {
        // 序列化页面数据
        char page_data[PAGE_SIZE];
        it->second.page.serialize(page_data);

        // 写入磁盘
        disk_manager_->writePage(page_id, page_data);

        it->second.is_dirty = false;
        it->second.page.setDirty(false);
    }
}

void BufferManager::flushAllPages() {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    for (auto& pair : page_table_) {
        if (pair.second.is_dirty) {
            // 序列化页面数据
            char page_data[PAGE_SIZE];
            pair.second.page.serialize(page_data);

            // 写入磁盘
            disk_manager_->writePage(pair.first, page_data);

            pair.second.is_dirty = false;
            pair.second.page.setDirty(false);
        }
    }
}

void BufferManager::removePage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return; // 页面不在缓冲池中
    }

    if (it->second.pin_count > 0) {
        throw BufferPoolFullException("Cannot remove pinned page: " + std::to_string(page_id));
    }

    // 如果是脏页，先刷新到磁盘
    if (it->second.is_dirty) {
        char page_data[PAGE_SIZE];
        it->second.page.serialize(page_data);
        disk_manager_->writePage(page_id, page_data);
    }

    // 从LRU列表和页面表中移除
    lru_list_.erase(it->second.iterator);
    page_table_.erase(it);
}

double BufferManager::getHitRate() const {
    size_t total = hit_count_ + miss_count_;
    if (total == 0) return 0.0;
    return static_cast<double>(hit_count_) / total;
}

bool BufferManager::evictPage() {
    // 根据替换策略选择要淘汰的页面
    PageID page_to_evict = INVALID_PAGE_ID;

    if (policy_ == BufferReplacementPolicy::LRU) {
        // 从LRU列表末尾开始查找未被固定的页面
        for (auto it = lru_list_.rbegin(); it != lru_list_.rend(); ++it) {
            auto frame_it = page_table_.find(*it);
            if (frame_it != page_table_.end() && frame_it->second.pin_count == 0) {
                page_to_evict = *it;
                break;
            }
        }
    } else { // FIFO
        // 从LRU列表末尾开始查找未被固定的页面
        for (auto it = lru_list_.rbegin(); it != lru_list_.rend(); ++it) {
            auto frame_it = page_table_.find(*it);
            if (frame_it != page_table_.end() && frame_it->second.pin_count == 0) {
                page_to_evict = *it;
                break;
            }
        }
    }

    if (page_to_evict == INVALID_PAGE_ID) {
        return false; // 没有可淘汰的页面
    }

    // 移除页面
    removePage(page_to_evict);
    return true;
}

void BufferManager::updateAccessTime(PageID page_id) {
    if (policy_ == BufferReplacementPolicy::LRU) {
        auto it = page_table_.find(page_id);
        if (it != page_table_.end()) {
            // 从LRU列表中移除并重新添加到头部
            lru_list_.erase(it->second.iterator);
            lru_list_.push_front(page_id);
            it->second.iterator = lru_list_.begin();
        }
    }
    // FIFO策略不需要更新访问时间
}

} // namespace storage
} // namespace minidb