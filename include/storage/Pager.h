#ifndef MINIDB_PAGER_H
#define MINIDB_PAGER_H

#include "storage/BufferManager.h"
#include "storage/DiskManager.h"
#include "storage/Page.h"
#include <memory>
#include <set>

namespace minidb {
    namespace storage {

        class Pager {
        public:
            Pager(std::shared_ptr<DiskManager> disk_manager,
                  std::shared_ptr<BufferManager> buffer_manager);

            PageID allocatePage();
            void deallocatePage(PageID page_id);

            Page* getPage(PageID page_id);
            void pinPage(PageID page_id);
            void releasePage(PageID page_id, bool is_dirty = false);

            void flushPage(PageID page_id);
            void flushAll();

            PageID getPageCount() const;
            bool isPageInUse(PageID page_id) const;

        private:
            std::shared_ptr<DiskManager> disk_manager_;
            std::shared_ptr<BufferManager> buffer_manager_;
            std::set<PageID> allocated_pages_;
            mutable std::mutex pager_mutex_;

            void checkpoint();
        };

    } // namespace storage
} // namespace minidb

#endif // MINIDB_PAGER_H