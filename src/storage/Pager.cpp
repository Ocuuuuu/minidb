#include "../include/storage/Pager.h"
#include <stdexcept>
#include <iostream>

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
            // 检查页面ID是否有效
            if (page_id == INVALID_PAGE_ID || page_id < 0) {
                return;
            }

            std::lock_guard<std::mutex> lock(pager_mutex_);

            // 检查页面是否真的被分配过
            if (allocated_pages_.find(page_id) == allocated_pages_.end()) {
                return;
            }

            try {
                // 尝试从缓冲池中移除页面（如果存在）
                buffer_manager_->removePage(page_id);
            } catch (const std::exception& e) {
                // 如果页面不在缓冲池中，记录警告但继续执行
                // 这可能是正常情况，因为页面可能从未被访问过
                std::cerr << "Warning: Page " << page_id
                          << " not in buffer pool during deallocation: "
                          << e.what() << std::endl;
            }

            // 从磁盘中释放页面
            disk_manager_->deallocatePage(page_id);

            // 从已分配页面集合中移除
            allocated_pages_.erase(page_id);
        }

        Page* Pager::getPage(PageID page_id) {
            if (!isValidPage(page_id)) {
                throw std::runtime_error("Invalid page ID");
            }

            return buffer_manager_->fetchPage(page_id);
        }

        void Pager::pinPage(PageID page_id) {
            if (!isValidPage(page_id)) {
                throw std::runtime_error("Invalid page ID");
            }

            buffer_manager_->pinPage(page_id);
        }

        void Pager::releasePage(PageID page_id, bool is_dirty) {
            if (!isValidPage(page_id)) {
                throw std::runtime_error("Invalid page ID");
            }

            try {
                buffer_manager_->unpinPage(page_id, is_dirty);
            } catch (const std::exception& e) {
                // 处理页面不在缓冲池中的情况
                std::cerr << "Warning: Page " << page_id
                          << " not in buffer pool during release: "
                          << e.what() << std::endl;
                // 可以选择抛出异常或静默处理
                // throw std::runtime_error("Failed to release page: " + std::string(e.what()));
            }
        }

        void Pager::flushPage(PageID page_id) {
            if (!isValidPage(page_id)) {
                throw std::runtime_error("Invalid page ID");
            }

            buffer_manager_->flushPage(page_id);
        }

        void Pager::flushAll() {
            buffer_manager_->flushAllPages();
        }

        PageID Pager::getPageCount() const {
            return disk_manager_->getPageCount();
        }

        bool Pager::isPageInUse(PageID page_id) const {
            if (!isValidPage(page_id)) {
                return false;
            }

            std::lock_guard<std::mutex> lock(pager_mutex_);
            return allocated_pages_.find(page_id) != allocated_pages_.end();
        }

        bool Pager::isValidPage(PageID page_id) const {
            // 检查页面ID是否有效
            if (page_id == INVALID_PAGE_ID || page_id < 0) {
                return false;
            }

            // 检查页面ID是否在合理范围内
            PageID page_count = getPageCount();
            if (page_id >= page_count) {
                return false;
            }

            // 额外的检查：页面是否真的被Pager分配过
            std::lock_guard<std::mutex> lock(pager_mutex_);
            return allocated_pages_.find(page_id) != allocated_pages_.end();
        }

        void Pager::checkpoint() {
            // 简易版中暂不实现检查点功能
        }

    } // namespace storage
} // namespace minidb