//
// Created by tang_ on 2025/9/9.
//

#include "../../include/storage/DiskManager.h"
#include "storage/DiskManager.h"
#include <cstring>
#include <iostream>

namespace minidb {
namespace storage {

DiskManager::DiskManager(FileManager* file_manager)
    : file_manager_(file_manager) {
    std::cout << "DiskManager constructor started" << std::endl;
    if (file_manager_->isOpen()) {
        std::cout << "File manager is open, reading header" << std::endl;
        readHeader();
        std::cout << "Header read, page count: " << page_count_ << std::endl;
    } else {
        std::cout << "File manager is not open" << std::endl;
        page_count_ = 0;
    }
    std::cout << "DiskManager constructor completed" << std::endl;
}

DiskManager::~DiskManager() {
    if (file_manager_->isOpen()) {
        writeHeader();
    }
}

void DiskManager::readPage(PageID page_id, char* data) {
    std::lock_guard<std::mutex> lock(io_mutex_);

    if (page_id >= page_count_) {
        throw DatabaseException("Page id out of range: " + std::to_string(page_id));
    }

    auto& file = file_manager_->getFileStream();
    file.seekg(page_id * PAGE_SIZE, std::ios::beg);
    file.read(data, PAGE_SIZE);

    if (file.fail()) {
        throw DatabaseException("Failed to read page: " + std::to_string(page_id));
    }
}

void DiskManager::writePage(PageID page_id, const char* data) {
    std::cout << "writePage started for page: " << page_id << std::endl;
    // 添加调试信息，检查锁的状态
    std::cout << "Attempting to acquire lock..." << std::endl;
    std::lock_guard<std::mutex> lock(io_mutex_);
    std::cout << "Lock acquired successfully" << std::endl;

    std::cout << "writePage lock acquired" << std::endl;

    if (page_id >= page_count_) {
        std::cout << "Page id out of range: " << page_id << std::endl;
        throw DatabaseException("Page id out of range: " + std::to_string(page_id));
    }

    try {
        auto& file = file_manager_->getFileStream();
        std::cout << "Got file stream" << std::endl;

        // 检查文件状态
        if (!file.is_open()) {
            std::cout << "File is not open" << std::endl;
            throw DatabaseException("File is not open");
        }

        if (file.fail()) {
            std::cout << "File is in fail state" << std::endl;
            throw DatabaseException("File is in fail state");
        }

        file.seekp(page_id * PAGE_SIZE, std::ios::beg);
        std::cout << "Seek to position: " << (page_id * PAGE_SIZE) << std::endl;

        file.write(data, PAGE_SIZE);
        std::cout << "Data written" << std::endl;

        if (file.fail()) {
            std::cout << "File write failed" << std::endl;
            throw DatabaseException("Failed to write page: " + std::to_string(page_id));
        }

        file.flush();
        std::cout << "File flushed" << std::endl;

        std::cout << "writePage completed for page: " << page_id << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Exception in writePage: " << e.what() << std::endl;
        throw;
    }
}

PageID DiskManager::allocatePage() {
    std::cout << "allocatePage started" << std::endl;

    PageID new_page_id;
    {
        std::lock_guard<std::mutex> lock(io_mutex_);
        std::cout << "Lock acquired" << std::endl;

        new_page_id = page_count_;
        std::cout << "New page ID: " << new_page_id << std::endl;

        page_count_++;
        std::cout << "Page count increased to: " << page_count_ << std::endl;
    } // 释放锁

    // 初始化新页面（不在锁的保护范围内）
    char empty_page[PAGE_SIZE] = {0};
    writePage(new_page_id, empty_page);
    std::cout << "Empty page written" << std::endl;

    // 更新文件头（writeHeader内部会获取锁）
    writeHeader();
    std::cout << "Header updated" << std::endl;

    std::cout << "allocatePage completed, returning: " << new_page_id << std::endl;
    return new_page_id;
}

void DiskManager::deallocatePage(PageID page_id) {
    std::lock_guard<std::mutex> lock(io_mutex_);

    if (page_id >= page_count_) {
        throw DatabaseException("Page id out of range: " + std::to_string(page_id));
    }

    // 在实际系统中，这里应该将页面标记为空闲并加入空闲列表
    // 简化版本中，我们只是减少页面计数（这不完全正确，但作为起点）
    // 注意：这只是一个简化实现，实际系统需要更复杂的管理

    page_count_--;
    writeHeader();
}

void DiskManager::flush() {
    std::lock_guard<std::mutex> lock(io_mutex_);
    file_manager_->getFileStream().flush();
}

PageID DiskManager::getPageCount() const {
    return page_count_;
}

void DiskManager::readHeader() {
    char header_page[PAGE_SIZE];
    auto& file = file_manager_->getFileStream();

    file.seekg(0, std::ios::beg);
    file.read(header_page, PAGE_SIZE);

    if (file.fail()) {
        throw DatabaseException("Failed to read header page");
    }

    // 从文件头中解析出页数
    std::memcpy(&page_count_, header_page, sizeof(PageID));
}

void DiskManager::writeHeader() {
    std::cout << "writeHeader started" << std::endl;

    std::lock_guard<std::mutex> lock(io_mutex_);
    std::cout << "writeHeader lock acquired" << std::endl;

    try {
        char header_page[PAGE_SIZE] = {0};
        auto& file = file_manager_->getFileStream();
        std::cout << "Got file stream for header" << std::endl;

        // 检查文件状态
        if (!file.is_open()) {
            std::cout << "File is not open" << std::endl;
            throw DatabaseException("File is not open");
        }

        if (file.fail()) {
            std::cout << "File is in fail state" << std::endl;
            throw DatabaseException("File is in fail state");
        }

        // 将页数写入文件头
        std::memcpy(header_page, &page_count_, sizeof(PageID));
        std::cout << "Page count copied to header: " << page_count_ << std::endl;

        file.seekp(0, std::ios::beg);
        std::cout << "Seek to beginning" << std::endl;

        file.write(header_page, PAGE_SIZE);
        std::cout << "Header written" << std::endl;

        file.flush();
        std::cout << "File flushed" << std::endl;

        if (file.fail()) {
            std::cout << "Header write failed" << std::endl;
            throw DatabaseException("Failed to write header");
        }

        std::cout << "writeHeader completed" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Exception in writeHeader: " << e.what() << std::endl;
        throw;
    }
}

} // namespace storage
} // namespace minidbnamespace minidb