#include "../include/storage/DiskManager.h"

#include <cstring>
#include <iostream>

namespace minidb {
namespace storage {

DiskManager::DiskManager(std::shared_ptr<FileManager> file_manager)
    : file_manager_(file_manager) {
    // 读取头部信息
    readHeader();
}

DiskManager::~DiskManager() {
    // 确保头部信息被写入
    writeHeader();
}

void DiskManager::readPage(PageID page_id, char* data) {
    if (page_id >= page_count_) {
        throw DiskException("Page ID out of range: " + std::to_string(page_id));
    }

    std::lock_guard<std::mutex> lock(io_mutex_);
    auto& file = file_manager_->getFileStream();

    // 计算偏移量
    std::streampos offset = page_id * PAGE_SIZE;
    file.seekg(offset);

    if (!file.read(data, PAGE_SIZE)) {
        throw DiskException("Failed to read page: " + std::to_string(page_id));
    }
}

void DiskManager::writePage(PageID page_id, const char* data) {
    if (page_id >= page_count_) {
        throw DiskException("Page ID out of range: " + std::to_string(page_id));
    }

    std::lock_guard<std::mutex> lock(io_mutex_);
    auto& file = file_manager_->getFileStream();

    // 计算偏移量
    std::streampos offset = page_id * PAGE_SIZE;
    file.seekp(offset);

    if (!file.write(data, PAGE_SIZE)) {
        throw DiskException("Failed to write page: " + std::to_string(page_id));
    }

    // 确保数据被写入
    file.flush();
}

PageID DiskManager::allocatePage() {
    std::lock_guard<std::mutex> lock(io_mutex_);

    PageID allocated_page;

    if (free_list_head_ != INVALID_PAGE_ID) {
        // 从空闲链表分配页面
        allocated_page = free_list_head_;

        // 读取空闲页面头信息以获取下一个空闲页面
        char page_data[PAGE_SIZE];
        readPage(free_list_head_, page_data);

        // 解析下一个空闲页面ID
        PageID next_free = *reinterpret_cast<PageID*>(page_data);
        free_list_head_ = next_free;
    } else {
        // 分配新页面
        allocated_page = page_count_;
        page_count_++;

        // 扩展文件大小
        auto& file = file_manager_->getFileStream();
        file.seekp(0, std::ios::end);
        std::streampos file_size = file.tellp();

        if (file_size < static_cast<std::streampos>((allocated_page + 1) * PAGE_SIZE)) {
            file.seekp((allocated_page + 1) * PAGE_SIZE - 1);
            file.put('\0');
        }
    }

    // 更新头部信息
    writeHeader();

    return allocated_page;
}

void DiskManager::deallocatePage(PageID page_id) {
    if (page_id >= page_count_ || page_id == 0) {
        throw DiskException("Invalid page ID for deallocation: " + std::to_string(page_id));
    }

    std::lock_guard<std::mutex> lock(io_mutex_);

    // 将页面添加到空闲链表头部
    char page_data[PAGE_SIZE];
    std::memset(page_data, 0, PAGE_SIZE);

    // 将当前空闲链表头存储到被释放的页面中
    *reinterpret_cast<PageID*>(page_data) = free_list_head_;
    writePage(page_id, page_data);

    // 更新空闲链表头
    free_list_head_ = page_id;

    // 更新头部信息
    writeHeader();
}

void DiskManager::flush() {
    std::lock_guard<std::mutex> lock(io_mutex_);
    auto& file = file_manager_->getFileStream();
    file.flush();
}

PageID DiskManager::getPageCount() const {
    return page_count_;
}

void DiskManager::readHeader() {
    if (!file_manager_->isOpen()) return;

    std::lock_guard<std::mutex> lock(io_mutex_);
    auto& file = file_manager_->getFileStream();

    // 检查文件大小
    file.seekg(0, std::ios::end);
    std::streampos file_size = file.tellg();

    if (file_size == 0) {
        // 新文件，初始化头部
        page_count_ = 1;
        free_list_head_ = INVALID_PAGE_ID;
        writeHeader();
        return;
    }

    // 读取头部信息
    file.seekg(0);
    file.read(reinterpret_cast<char*>(&page_count_), sizeof(PageID));
    file.read(reinterpret_cast<char*>(&free_list_head_), sizeof(PageID));
}

void DiskManager::writeHeader() {
    if (!file_manager_->isOpen()) return;

    std::lock_guard<std::mutex> lock(io_mutex_);
    auto& file = file_manager_->getFileStream();

    file.seekp(0);
    file.write(reinterpret_cast<const char*>(&page_count_), sizeof(PageID));
    file.write(reinterpret_cast<const char*>(&free_list_head_), sizeof(PageID));
    file.flush();
}

} // namespace storage
} // namespace minidb