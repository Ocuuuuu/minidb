#include "../../include/storage/Pager.h"
#include "../../include/common/Exception.h"
#include "../../include/storage/DiskManager.h"

#include "../../include/common/Constants.h"
#include "../../include/storage/BufferManager.h"
namespace minidb {
namespace storage {

// ====================== 构造函数 ======================
Pager::Pager(std::shared_ptr<DiskManager> disk_manager,
             std::shared_ptr<BufferManager> buffer_manager)
    : disk_manager_(std::move(disk_manager)),
      buffer_manager_(std::move(buffer_manager)) {
    if (!disk_manager_) {
        throw DatabaseException("DiskManager is null in Pager constructor");
    }
    if (!buffer_manager_) {
        throw DatabaseException("BufferManager is null in Pager constructor");
    }

    // 初始化已分配页集合（从磁盘管理器加载当前已分配的页）
    PageID total_pages = disk_manager_->getPageCount();
    PageID free_head = disk_manager_->getFreeListHead();

    // 标记所有非空闲页为已分配
    for (PageID i = 1; i < total_pages; ++i) {  // 跳过0号元数据页
        if (i != free_head) {
            allocated_pages_.insert(i);
        }
    }
}

// ====================== 页分配与释放 ======================
PageID Pager::allocatePage() {
    std::lock_guard<std::mutex> lock(pager_mutex_);

    // 调用磁盘管理器分配页
    PageID new_page_id = disk_manager_->allocatePage();
    if (new_page_id == INVALID_PAGE_ID) {
        throw PageException("Failed to allocate new page");
    }

    // 记录到已分配集合
    allocated_pages_.insert(new_page_id);
    return new_page_id;
}

void Pager::deallocatePage(PageID page_id) {
    std::lock_guard<std::mutex> lock(pager_mutex_);

    // 检查页是否存在且已分配
    if (!isPageInUse(page_id)) {
        throw InvalidRIDException(RID{page_id, -1});
    }

    // 从缓冲池移除页（确保后续操作基于磁盘最新状态）
    buffer_manager_->removePage(page_id);

    // 调用磁盘管理器释放页
    disk_manager_->deallocatePage(page_id);

    // 从已分配集合移除
    allocated_pages_.erase(page_id);
}

// ====================== 页访问与管理 ======================
Page* Pager::getPage(PageID page_id) {
    std::lock_guard<std::mutex> lock(pager_mutex_);

    if (!isPageInUse(page_id)) {
        throw PageNotInPoolException(page_id);
    }

    // 从缓冲池获取页（内部会处理磁盘加载）
    return buffer_manager_->fetchPage(page_id);
}

void Pager::pinPage(PageID page_id) {
    std::lock_guard<std::mutex> lock(pager_mutex_);

    if (!isPageInUse(page_id)) {
        throw PageNotInPoolException(page_id);
    }

    buffer_manager_->pinPage(page_id);
}

void Pager::releasePage(PageID page_id, bool is_dirty) {
    std::lock_guard<std::mutex> lock(pager_mutex_);

    if (!isPageInUse(page_id)) {
        throw PageNotInPoolException(page_id);
    }

    buffer_manager_->unpinPage(page_id, is_dirty);
}

// ====================== 刷盘操作 ======================
void Pager::flushPage(PageID page_id) {
    std::lock_guard<std::mutex> lock(pager_mutex_);

    if (!isPageInUse(page_id)) {
        throw PageNotInPoolException(page_id);
    }

    buffer_manager_->flushPage(page_id);
}

void Pager::flushAll() {
    std::lock_guard<std::mutex> lock(pager_mutex_);
    buffer_manager_->flushAllPages();
}

// ====================== 页状态查询 ======================
PageID Pager::getPageCount() const {
    std::lock_guard<std::mutex> lock(pager_mutex_);
    return disk_manager_->getPageCount();
}

bool Pager::isPageInUse(PageID page_id) const {
    std::lock_guard<std::mutex> lock(pager_mutex_);
    // 有效页ID范围：1 ~ (总页数-1)（0号是元数据页）
    return (page_id > 0 && page_id < disk_manager_->getPageCount())
        && allocated_pages_.count(page_id) > 0;
}

// ====================== 检查点（数据一致性保障） ======================
void Pager::checkpoint() {
    std::lock_guard<std::mutex> lock(pager_mutex_);
    // 刷写所有脏页到磁盘，确保内存与磁盘状态一致
    flushAll();
    // 持久化磁盘管理器的元数据（页计数、空闲链表）
    disk_manager_->flush();
}

} // namespace storage
} // namespace minidb
