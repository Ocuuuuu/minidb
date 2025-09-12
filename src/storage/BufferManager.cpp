//
// Created by tang_ on 2025/9/9.
//

#include "../../include/storage/BufferManager.h"
#include <iostream>

namespace minidb {
namespace storage {

BufferManager::BufferManager(DiskManager* disk_manager, size_t pool_size,
                           BufferReplacementPolicy policy)
    : disk_manager_(disk_manager), pool_size_(pool_size), policy_(policy) {
    std::cout << "BufferManager constructor started" << std::endl;
    std::cout << "Disk manager: " << disk_manager_ << std::endl;
    std::cout << "Pool size: " << pool_size_ << std::endl;
    std::cout << "BufferManager constructor completed" << std::endl;
}

BufferManager::~BufferManager() {
    flushAllPages();
}

Page* BufferManager::fetchPage(PageID page_id) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    // 首先在缓存中查找
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        // 找到页面，更新访问时间
        updateAccessTime(page_id);
        hit_count_++;
        return &(it->second.first);
    }

    // 未找到，增加未命中计数
    miss_count_++;

    // 如果缓存已满，需要替换页面
    if (page_table_.size() >= pool_size_) {
        evictPage();
    }

    // 从磁盘读取页面
    Page new_page(page_id);
    disk_manager_->readPage(page_id, new_page.getData());

    // 将新页面加入缓存
    lru_list_.push_front(page_id);
    page_table_[page_id] = std::make_pair(new_page, lru_list_.begin());

    return &(page_table_[page_id].first);
}

void BufferManager::unpinPage(PageID page_id, bool is_dirty) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        it->second.first.setDirty(is_dirty);
    }
}

void BufferManager::flushPage(PageID page_id) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    auto it = page_table_.find(page_id);
    if (it != page_table_.end() && it->second.first.isDirty()) {
        disk_manager_->writePage(page_id, it->second.first.getData());
        it->second.first.setDirty(false);
    }
}

void BufferManager::flushAllPages() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    for (auto& entry : page_table_) {
        if (entry.second.first.isDirty()) {
            disk_manager_->writePage(entry.first, entry.second.first.getData());
            entry.second.first.setDirty(false);
        }
    }
}

double BufferManager::getHitRate() const {
    size_t total = hit_count_ + miss_count_;
    if (total == 0) return 0.0;
    return static_cast<double>(hit_count_) / total;
}

void BufferManager::evictPage() {
    if (lru_list_.empty()) return;

    PageID victim_page_id;

    if (policy_ == BufferReplacementPolicy::LRU) {
        // LRU策略：淘汰最近最少使用的页面
        victim_page_id = lru_list_.back();
        lru_list_.pop_back();
    } else {
        // FIFO策略：淘汰最早进入的页面
        victim_page_id = lru_list_.back();
        lru_list_.pop_back();
    }

    auto it = page_table_.find(victim_page_id);
    if (it != page_table_.end()) {
        if (it->second.first.isDirty()) {
            disk_manager_->writePage(victim_page_id, it->second.first.getData());
        }
        page_table_.erase(it);
    }
}

void BufferManager::updateAccessTime(PageID page_id) {
    // 更新页面在LRU列表中的位置
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        lru_list_.erase(it->second.second);
        lru_list_.push_front(page_id);
        it->second.second = lru_list_.begin();
    }
}

} // namespace storage
} // namespace minidb} // namespace minidb