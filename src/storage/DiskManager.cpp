#include "../include/storage/DiskManager.h"

#include <cstring>
#include <iostream>
#include <vector>
#include <common/Exception.h>
#include <filesystem>

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

    void DiskManager::readPage(PageID page_id, char* data, bool require_lock) {
    if (page_id >= page_count_) {
        throw DiskException("Page ID out of range: " + std::to_string(page_id));
    }

    std::unique_lock<std::mutex> lock(io_mutex_, std::defer_lock);
    if (require_lock) {
        lock.lock();
    }

    auto& file = file_manager_->getFileStream();
    std::streampos offset = page_id * PAGE_SIZE;
    file.seekg(offset);


    // 检查文件流状态
    if (!file.good()) {
        std::cout << "File stream not good before read, state: "
                  << " good=" << file.good()
                  << " eof=" << file.eof()
                  << " fail=" << file.fail()
                  << " bad=" << file.bad() << std::endl;
        throw DiskException("File stream is not in a good state before reading the page.");
    }


    if (!file.read(data, PAGE_SIZE)) {
        throw DiskException("Failed to read page: " + std::to_string(page_id));
    }
}

    void DiskManager::writePage(PageID page_id, const char* data, bool require_lock) {
    if (page_id >= page_count_) {
        throw DiskException("Page ID out of range: " + std::to_string(page_id));
    }

    std::unique_lock<std::mutex> lock(io_mutex_, std::defer_lock);
    if (require_lock) {
        lock.lock();
    }

    auto& file = file_manager_->getFileStream();
    std::streampos offset = page_id * PAGE_SIZE;
    file.seekp(offset);

    if (!file.write(data, PAGE_SIZE)) {
        throw DiskException("Failed to write page: " + std::to_string(page_id));
    }

    file.flush();
}

    PageID DiskManager::allocatePage() {
    std::cout << "allocatePage() called" << std::endl;

    std::lock_guard<std::mutex> lock(io_mutex_);
    std::cout << "Lock acquired, free_list_head_: " << free_list_head_ << std::endl;

    PageID allocated_page;

    if (free_list_head_ != INVALID_PAGE_ID) {
        std::cout << "Allocating from free list: " << free_list_head_ << std::endl;

        // 从空闲链表分配页面
        allocated_page = free_list_head_;

        // 直接读取页面内容
        char page_data[PAGE_SIZE];
        auto& file = file_manager_->getFileStream();
        std::streampos offset = free_list_head_ * PAGE_SIZE;

        std::cout << "Seeking to offset: " << offset << std::endl;
        file.seekg(offset);

        std::cout << "Reading page data..." << std::endl;
        if (!file.read(page_data, PAGE_SIZE)) {
            throw DiskException("Failed to read free page: " + std::to_string(free_list_head_));
        }

        // 解析下一个空闲页面ID
        PageID next_free = *reinterpret_cast<PageID*>(page_data);
        std::cout << "Next free page: " << next_free << std::endl;
        free_list_head_ = next_free;
    } else {
        std::cout << "Allocating new page, current page_count_: " << page_count_ << std::endl;

        // 分配新页面
        allocated_page = page_count_;
        page_count_++;

        // 扩展文件大小
        auto& file = file_manager_->getFileStream();
        file.seekp(0, std::ios::end);
        std::streampos file_size = file.tellp();

        std::cout << "Current file size: " << file_size
                  << ", required size: " << ((allocated_page + 1) * PAGE_SIZE) << std::endl;

        if (file_size < static_cast<std::streampos>((allocated_page + 1) * PAGE_SIZE)) {
            std::cout << "Extending file..." << std::endl;

            // 方法1：使用 std::filesystem::resize_file（推荐）
            try {
                // 先刷新并关闭当前文件流
                std::cout << "Flushing and closing file..." << std::endl;
                file.flush();
                file.close();

                // 使用 filesystem 库调整文件大小
                std::string db_path = file_manager_->getDatabasePath();
                std::cout << "Resizing file to " << (allocated_page + 1) * PAGE_SIZE << " bytes..." << std::endl;
                std::filesystem::resize_file(db_path, (allocated_page + 1) * PAGE_SIZE);

                // 重新打开文件
                std::cout << "Reopening file..." << std::endl;
                file.open(db_path, std::ios::in | std::ios::out | std::ios::binary);
                if (!file.is_open()) {
                    throw DiskException("Failed to reopen file after resizing");
                }

                // 检查重新打开后的文件状态
                std::cout << "File reopened successfully, state: "
                          << " good=" << file.good()
                          << " eof=" << file.eof()
                          << " fail=" << file.fail()
                          << " bad=" << file.bad() << std::endl;

                // 重置文件指针
                file.seekp(0);
                file.clear();

                std::cout << "File position after reopen and seek: " << file.tellp() << std::endl;

                std::cout << "File resized to " << (allocated_page + 1) * PAGE_SIZE << " bytes using filesystem" << std::endl;
            } catch (const std::exception& e) {
                throw DiskException("Failed to resize file: " + std::string(e.what()));
            }
        } else {
            std::cout << "No file extension needed" << std::endl;
        }
    }

    std::cout << "Writing header..." << std::endl;
    writeHeader(false);  // 修改这里：明确传递 false 参数
    std::cout << "Allocated page: " << allocated_page << std::endl;

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
    writePage(page_id, page_data, false);  // 不获取锁

    // 更新空闲链表头
    free_list_head_ = page_id;

    writeHeader(false);  // 修改这里：明确传递 false 参数
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
    std::cout << "readHeader() called" << std::endl;

    if (!file_manager_->isOpen()) {
        std::cout << "File not open, using default values" << std::endl;
        page_count_ = 1;
        free_list_head_ = INVALID_PAGE_ID;
        return;
    }

    // 先检查文件大小，不持有锁
    std::streampos file_size;
    {
        std::cout << "Checking file size..." << std::endl;
        std::lock_guard<std::mutex> lock(io_mutex_);
        auto& file = file_manager_->getFileStream();
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
        std::cout << "File size: " << file_size << " bytes" << std::endl;
        file.seekg(0); // 重置文件指针
    }

    if (file_size == 0) {
        std::cout << "New file detected, initializing header" << std::endl;
        page_count_ = 1;
        free_list_head_ = INVALID_PAGE_ID;
        writeHeader();

        // 用空数据初始化第一页以确保页存在
        char empty_data[PAGE_SIZE] = {0};
        writePage(0, empty_data, false);  // 不获取锁

        std::cout << "Header initialized successfully" << std::endl;
        return;
    }

    std::cout << "Reading existing header..." << std::endl;

    // 读取头部信息
    std::lock_guard<std::mutex> lock(io_mutex_);
    auto& file = file_manager_->getFileStream();

    file.seekg(0);
    std::cout << "File position after seek: " << file.tellg() << std::endl;

    if (!file.read(reinterpret_cast<char*>(&page_count_), sizeof(PageID))) {
        std::cout << "Failed to read page_count_" << std::endl;
        throw DiskException("Failed to read page_count from header");
    }

    if (!file.read(reinterpret_cast<char*>(&free_list_head_), sizeof(PageID))) {
        std::cout << "Failed to read free_list_head_" << std::endl;
        throw DiskException("Failed to read free_list_head from header");
    }

    std::cout << "Header read successfully: page_count=" << page_count_
              << ", free_list_head=" << free_list_head_ << std::endl;
}

    // 在源文件中修改实现：
void DiskManager::writeHeader(bool require_lock) {
    std::cout << "writeHeader() - Entering method, require_lock: " << require_lock << std::endl;

    if (!file_manager_->isOpen()) {
        std::cout << "File is not open, skipping header write" << std::endl;
        return;
    }

    std::unique_lock<std::mutex> lock(io_mutex_, std::defer_lock);
    if (require_lock) {
        std::cout << "writeHeader() - Attempting to acquire lock" << std::endl;
        lock.lock();
        std::cout << "writeHeader() - Lock acquired" << std::endl;
    } else {
        std::cout << "writeHeader() - Skipping lock acquisition" << std::endl;
    }

    auto& file = file_manager_->getFileStream();

    std::cout << "writeHeader() called, file state: "
              << " good=" << file.good()
              << " eof=" << file.eof()
              << " fail=" << file.fail()
              << " bad=" << file.bad() << std::endl;

    // 彻底清除错误状态
    file.clear();

    // 检查文件指针位置
    std::cout << "File position before seek: " << file.tellp() << std::endl;
    file.seekp(0);
    std::cout << "File position after seek: " << file.tellp() << std::endl;

    // 检查文件流状态
    if (!file.good()) {
        std::cout << "File stream is not good after seek" << std::endl;
        throw DiskException("File stream is not in good state");
    }

    std::cout << "Writing page_count: " << page_count_ << std::endl;
    file.write(reinterpret_cast<const char*>(&page_count_), sizeof(PageID));

    // 立即检查写入状态
    if (!file) {
        std::cout << "Failed to write page_count, error: "
                  << " good=" << file.good()
                  << " eof=" << file.eof()
                  << " fail=" << file.fail()
                  << " bad=" << file.bad() << std::endl;
        throw DiskException("Failed to write page_count to header");
    }

    std::cout << "Writing free_list_head: " << free_list_head_ << std::endl;
    file.write(reinterpret_cast<const char*>(&free_list_head_), sizeof(PageID));

    if (!file) {
        std::cout << "Failed to write free_list_head" << std::endl;
        throw DiskException("Failed to write free_list_head to header");
    }

    // 强制刷新
    file.flush();

    if (!file) {
        std::cout << "Failed to flush after header write" << std::endl;
        throw DiskException("Failed to flush header");
    }

    std::cout << "Header written successfully" << std::endl;
}

} // namespace storage
} // namespace minidb