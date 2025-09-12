//
// Created by tang_ on 2025/9/9.
//

#ifndef MINIDB_BUFFERMANAGER_H
#define MINIDB_BUFFERMANAGER_H


#include "common/Constants.h"
#include "common/Exception.h"
#include "storage/Page.h"
#include "storage/DiskManager.h"
#include <list>
#include <unordered_map>
#include <mutex>

namespace minidb {
    namespace storage {

        // 缓冲区替换策略枚举
        enum class BufferReplacementPolicy {
            LRU,    // 最近最少使用
            FIFO    // 先进先出
        };

        class BufferManager {
        public:
            BufferManager(DiskManager* disk_manager,
                          size_t pool_size = DEFAULT_BUFFER_POOL_SIZE,
                          BufferReplacementPolicy policy = BufferReplacementPolicy::LRU);
            ~BufferManager();

            // 页面访问
            Page* fetchPage(PageID page_id);
            void unpinPage(PageID page_id, bool is_dirty = false);

            // 页面管理
            void flushPage(PageID page_id);
            void flushAllPages();

            // 统计信息
            size_t getHitCount() const { return hit_count_; }
            size_t getMissCount() const { return miss_count_; }
            double getHitRate() const;
            size_t getPoolSize() const { return pool_size_; }
            size_t getCurrentPages() const { return page_table_.size(); }

            // 策略配置
            void setReplacementPolicy(BufferReplacementPolicy policy) { policy_ = policy; }

        private:
            DiskManager* disk_manager_;
            size_t pool_size_;
            BufferReplacementPolicy policy_;

            // 缓存数据结构
            std::list<PageID> lru_list_;
            std::unordered_map<PageID, std::pair<Page, typename std::list<PageID>::iterator>> page_table_;

            // 统计信息
            size_t hit_count_{0};
            size_t miss_count_{0};
            mutable std::mutex buffer_mutex_;

            // 辅助函数
            void evictPage();
            void updateAccessTime(PageID page_id);
        };

    } // namespace storage
} // namespace minidb

#endif //MINIDB_BUFFERMANAGER_H