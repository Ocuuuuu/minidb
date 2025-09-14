#include "../include/storage/Pager.h"

namespace minidb {
    namespace storage {

        Pager::Pager(std::shared_ptr<DiskManager> disk_manager,
                     std::shared_ptr<BufferManager> buffer_manager)
            : disk_manager_(disk_manager), buffer_manager_(buffer_manager) {}

        PageID Pager::allocatePage() {
            std::lock_guard<std::mutex> lock(pager_mutex_);

            PageID page_id = disk_manager_->allocatePage();
            allocated_pages_.insert(page_id);

            return page_id;
        }

        void Pager::deallocatePage(PageID page_id) {
            std::lock_guard<std::mutex> lock(pager_mutex_);

            // 从缓冲池中移除页面
            buffer_manager_->removePage(page_id);

            // 从磁盘中释放页面
            disk_manager_->deallocatePage(page_id);

            // 从已分配页面集合中移除
            allocated_pages_.erase(page_id);
        }

        Page* Pager::getPage(PageID page_id) {
            return buffer_manager_->fetchPage(page_id);
        }

        void Pager::pinPage(PageID page_id) {
            buffer_manager_->pinPage(page_id);
        }

        void Pager::releasePage(PageID page_id, bool is_dirty) {
            buffer_manager_->unpinPage(page_id, is_dirty);
        }

        void Pager::flushPage(PageID page_id) {
            buffer_manager_->flushPage(page_id);
        }

        void Pager::flushAll() {
            buffer_manager_->flushAllPages();
        }

        PageID Pager::getPageCount() const {
            return disk_manager_->getPageCount();
        }

        bool Pager::isPageInUse(PageID page_id) const {
            std::lock_guard<std::mutex> lock(pager_mutex_);
            return allocated_pages_.find(page_id) != allocated_pages_.end();
        }

        void Pager::checkpoint() {
            // 简易版中暂不实现检查点功能
        }

    } // namespace storage
} // namespace minidb