#include "../../include/storage/BufferManager.h"
#include "../../include/storage/DiskManager.h"
#include "../../include/common/Constants.h"
#include "../../include/common/Exception.h"
#include <iostream>

namespace minidb {
namespace storage {

// ====================== 构造与析构 ======================
BufferManager::BufferManager(std::shared_ptr<DiskManager> disk_manager,
                             size_t pool_size)
    : disk_manager_(std::move(disk_manager)),
      pool_size_(pool_size) {
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
    std::cout << "fetchPage: " << page_id << std::endl;  // 添加日志
    // 检查页面是否已在缓冲池
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        updateAccessTime(page_id);
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

    // 创建缓冲帧并初始化（修正：直接操作页头的page_id）
    BufferFrame frame;
    frame.page.deserialize(buffer);
    frame.page.getHeader().page_id = page_id;  // 修正：通过页头设置page_id
    frame.pin_count = 1;
    frame.is_dirty = false;

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
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);  // 改为unique_lock

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

// ====================== 页面刷盘 ======================
void BufferManager::flushPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    if (it->second.is_dirty) {
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
        if (frame.is_dirty) {
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
    // 从LRU链表尾部开始查找（最久未使用）
    auto list_it = lru_list_.rbegin();
    while (list_it != lru_list_.rend()) {
        PageID evict_candidate = *list_it;
        auto table_it = page_table_.find(evict_candidate);

        // 处理已失效的链表节点
        if (table_it == page_table_.end()) {
            // 修正：规范迭代器转换和命名
            auto forward_it = --(list_it.base());  // 反向迭代器转正向迭代器
            list_it = std::make_reverse_iterator(lru_list_.erase(forward_it));  // 删除后更新迭代器
            continue;
        }

        BufferFrame& frame = table_it->second;

        // 跳过被固定的页面
        if (frame.pin_count > 0) {
            ++list_it;
            continue;
        }

        // 淘汰页面
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

    // 移到链表头部标记为最近访问
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