#ifndef MINIDB_BUFFERMANAGER_H
#define MINIDB_BUFFERMANAGER_H

#include "../../include/common/Constants.h"

#include "../../include/common/Exception.h"
#include "../../include/storage/Page.h"
#include "../../include/storage/DiskManager.h"
#include <list>
#include <unordered_map>
#include <shared_mutex>
#include <memory>

namespace minidb {
    namespace storage {

        enum class BufferReplacementPolicy {
            LRU,
        };

        class BufferManager {
        public:

            explicit BufferManager(std::shared_ptr<DiskManager> disk_manager,
             size_t pool_size = DEFAULT_BUFFER_POOL_SIZE);
                   ~BufferManager();

            BufferManager(const BufferManager&) = delete;
            BufferManager& operator=(const BufferManager&) = delete;

            Page* fetchPage(PageID page_id);
            void pinPage(PageID page_id);
            void unpinPage(PageID page_id, bool is_dirty = false);

            void flushPage(PageID page_id);
            void flushAllPages();
            void removePage(PageID page_id);

            size_t getHitCount() const { return hit_count_; }
            size_t getMissCount() const { return miss_count_; }
            double getHitRate() const;
            size_t getPoolSize() const { return pool_size_; }
            size_t getCurrentPages() const { return page_table_.size(); }

            void setReplacementPolicy(BufferReplacementPolicy policy) { policy_ = policy; }

            PageID allocatePage() {
                return disk_manager_->allocatePage();
            }

        private:
            std::shared_ptr<DiskManager> disk_manager_;
            size_t pool_size_;
            BufferReplacementPolicy policy_;

            struct BufferFrame {
                Page page;
                typename std::list<PageID>::iterator iterator;
                uint16_t pin_count;
                bool is_dirty;
            };

            std::list<PageID> lru_list_;
            std::unordered_map<PageID, BufferFrame> page_table_;

            size_t hit_count_{0};
            size_t miss_count_{0};
            mutable std::shared_mutex buffer_mutex_;

            bool evictPage();
            void updateAccessTime(PageID page_id);
        };

    } // namespace storage
} // namespace minidb

#endif // MINIDB_BUFFERMANAGER_H