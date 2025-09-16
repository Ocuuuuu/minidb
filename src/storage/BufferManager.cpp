// src/storage/BufferManager.cpp

#include "../../include/storage/BufferManager.h"
#include "../../include/storage/DiskManager.h"
#include "../../include/common/Constants.h"
#include "../../include/common/Exception.h"
#include <iostream>
#include <cstring>

namespace minidb {
namespace storage {

// ====================== 构造与析构 ======================
BufferManager::BufferManager(std::shared_ptr<DiskManager> disk_manager,
                             size_t pool_size)
    : disk_manager_(std::move(disk_manager)),
      pool_size_(pool_size),
      hit_count_(0),
      miss_count_(0) {
    if (pool_size_ == 0) {
        throw BufferPoolException("Buffer pool size cannot be zero");
    }
}

BufferManager::~BufferManager() {
    flushAllPages();
}

// ====================== 核心方法：获取页面 ======================
Page* BufferManager::fetchPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    // 检查页面是否已在缓冲池
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        updateAccessTime(page_id);
        it->second.pin_count++;
        hit_count_++;
        return &(it->second.page);
    }

    // 页面未命中，从磁盘加载
    miss_count_++;

    // 缓冲池已满时淘汰页面
    if (page_table_.size() >= pool_size_) {
        if (!evictPage()) {
            throw BufferPoolFullException("Cannot evict any page (all pinned)");
        }
    }

    // 从磁盘读取页面数据
    char buffer[PAGE_SIZE];
    disk_manager_->readPage(page_id, buffer);

    // 创建缓冲帧并反序列化
    BufferFrame frame;
    frame.page.deserialize(buffer);

    // ✅ 修复：正确判断是否为“全新页面”，避免覆盖已有数据
    auto& header = frame.page.getHeader();

    bool is_new_page = false;

    // 判断是否为新分配页面：slot_count=0 且 free_space 接近最大值 且 free_space_offset 合理
    if (header.slot_count == 0 &&
        (header.free_space_offset == 0 || header.free_space_offset == PAGE_SIZE) &&
        (header.free_space == 0 ||
         header.free_space == PAGE_SIZE - sizeof(PageHeader))) {

        // 进一步验证：如果 page_id 不匹配，可能是全新页面
        if (header.page_id == INVALID_PAGE_ID || header.page_id == 0) {
            is_new_page = true;
        }
    }

    if (is_new_page) {
        std::cout << "DEBUG: Initializing new page " << page_id << std::endl;

        // ✅ 正确初始化：记录从高地址向下生长
        header.page_id = page_id;
        header.page_type = PageType::DATA_PAGE;
        header.slot_count = 0;
        header.free_space_offset = PAGE_SIZE;  // ✅ 从高地址开始写入
        header.free_space = PAGE_SIZE - sizeof(PageHeader);
        header.is_dirty = true;                // header 被修改
        header.next_free_page = INVALID_PAGE_ID;

        std::cout << "DEBUG: Page initialized - free_space: " << header.free_space
                  << ", free_space_offset: " << header.free_space_offset << std::endl;
    } else {
        // ✅ 安全检查：确保 page_id 一致
        if (header.page_id != page_id) {
            std::cerr << "WARNING: Page ID mismatch: expected " << page_id
                      << ", but found " << header.page_id << ". Correcting."
                      << std::endl;
            header.page_id = page_id;
            header.is_dirty = true;
        }
    }

    frame.pin_count = 1;
    frame.is_dirty = header.is_dirty;

    // 加入LRU链表头部
    lru_list_.push_front(page_id);
    frame.iterator = lru_list_.begin();

    // 加入页表映射
    auto [inserted_it, success] = page_table_.emplace(page_id, std::move(frame));
    if (!success) {
        throw BufferPoolException("Failed to add page to buffer pool");
    }

    return &(inserted_it->second.page);
}

// ====================== 页面固定与解锁 ======================
void BufferManager::pinPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    it->second.pin_count++;
}

void BufferManager::unpinPage(PageID page_id, bool is_dirty) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    if (it->second.pin_count == 0) {
        throw BufferPoolException("Cannot unpin page " + std::to_string(page_id) + " (pin count is zero)");
    }
    it->second.pin_count--;

    if (is_dirty) {
        it->second.is_dirty = true;
        it->second.page.setDirty(true);
    }
}

void BufferManager::flushPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    bool is_dirty = it->second.is_dirty || it->second.page.isDirty();

    if (is_dirty) {
        char buffer[PAGE_SIZE];
        it->second.page.serialize(buffer);
        disk_manager_->writePage(page_id, buffer);

        it->second.is_dirty = false;
        it->second.page.setDirty(false);
    }
}

void BufferManager::flushAllPages() {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    for (auto& [page_id, frame] : page_table_) {
        bool is_dirty = frame.is_dirty || frame.page.isDirty();

        if (is_dirty) {
            char buffer[PAGE_SIZE];
            frame.page.serialize(buffer);
            disk_manager_->writePage(page_id, buffer);

            frame.is_dirty = false;
            frame.page.setDirty(false);
        }
    }
}

// ====================== 页面移除 ======================
void BufferManager::removePage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    if (it->second.pin_count > 0) {
        throw PinnedPageException(page_id);
    }

    if (it->second.is_dirty) {
        char buffer[PAGE_SIZE];
        it->second.page.serialize(buffer);
        disk_manager_->writePage(page_id, buffer);
    }

    lru_list_.erase(it->second.iterator);
    page_table_.erase(it);
}

// ====================== LRU淘汰策略 ======================
bool BufferManager::evictPage() {
    auto list_it = lru_list_.rbegin();
    while (list_it != lru_list_.rend()) {
        PageID evict_candidate = *list_it;
        auto table_it = page_table_.find(evict_candidate);

        if (table_it == page_table_.end()) {
            auto forward_it = --(list_it.base());
            list_it = std::make_reverse_iterator(lru_list_.erase(forward_it));
            continue;
        }

        BufferFrame& frame = table_it->second;

        if (frame.pin_count > 0) {
            ++list_it;
            continue;
        }

        if (frame.is_dirty) {
            char buffer[PAGE_SIZE];
            frame.page.serialize(buffer);
            disk_manager_->writePage(evict_candidate, buffer);
        }

        lru_list_.erase(frame.iterator);
        page_table_.erase(table_it);
        return true;
    }

    return false;
}

// ====================== 更新访问时间 ======================
void BufferManager::updateAccessTime(PageID page_id) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    lru_list_.erase(it->second.iterator);
    lru_list_.push_front(page_id);
    it->second.iterator = lru_list_.begin();
}

// ====================== 命中率计算 ======================
double BufferManager::getHitRate() const {
    std::shared_lock<std::shared_mutex> lock(buffer_mutex_);
    size_t total = hit_count_ + miss_count_;
    return total == 0 ? 0.0 : static_cast<double>(hit_count_) / total;
}

} // namespace storage
} // namespace minidb