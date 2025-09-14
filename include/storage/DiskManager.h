#ifndef MINIDB_DISKMANAGER_H
#define MINIDB_DISKMANAGER_H

#include "storage/FileManager.h"
#include "common/Types.h"
#include "common/Constants.h"
#include "common/Exception.h"
#include <mutex>
#include <memory>

namespace minidb {
    namespace storage {

        class DiskManager {
        public:
            explicit DiskManager(std::shared_ptr<FileManager> file_manager);
            ~DiskManager();

            DiskManager(const DiskManager&) = delete;
            DiskManager& operator=(const DiskManager&) = delete;

            void readPage(PageID page_id, char* data);
            void writePage(PageID page_id, const char* data);

            PageID allocatePage();
            void deallocatePage(PageID page_id);

            void flush();
            PageID getPageCount() const;
            PageID getFreeListHead() const { return free_list_head_; }

        private:
            std::shared_ptr<FileManager> file_manager_;
            PageID page_count_{1};
            PageID free_list_head_{INVALID_PAGE_ID};
            mutable std::mutex io_mutex_;

            void readHeader();
            void writeHeader();
        };

    } // namespace storage
} // namespace minidb

#endif // MINIDB_DISKMANAGER_H