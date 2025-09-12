//
// Created by tang_ on 2025/9/9.
//

#ifndef MINIDB_DISKMANAGER_H
#define MINIDB_DISKMANAGER_H
#include "storage/FileManager.h"
#include "common/Types.h"
#include "common/Constants.h"
#include "common/Exception.h"
#include <mutex>
namespace minidb {
    namespace storage {
        class DiskManager {
        public:
            //explicit禁止隐式转换
            explicit DiskManager(FileManager* file_manager);

            ~DiskManager();

            // 页面I/O操作
            void readPage(PageID page_id, char* data);

            void writePage(PageID page_id, const char* data);

            // 页面管理
            PageID allocatePage();
            void deallocatePage(PageID page_id);

            // 文件管理
            void flush();
            PageID getPageCount() const;

        private:
            FileManager* file_manager_;
            PageID page_count_{0};
            mutable std::mutex io_mutex_;

            // 元数据操作
            void readHeader();

            void writeHeader();

        };
    }
}



#endif //MINIDB_DISKMANAGER_H