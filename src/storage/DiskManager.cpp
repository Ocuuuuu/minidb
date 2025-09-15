#include "../../include/storage/DiskManager.h"
#include "../../include/common/Constants.h"
#include "../../include/storage/Page.h"
#include "../../include/common/Exception.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace minidb {
namespace storage {

DiskManager::DiskManager(std::shared_ptr<FileManager> file_manager)
    : file_manager_(std::move(file_manager)) {
    std::cout << "DiskManager constructor: Starting readHeader..." << std::endl;
    try {
        readHeader(); // 包裹 readHeader，避免构造函数中未处理的异常
    } catch (const DiskException& e) {
        std::cerr << "DiskManager constructor: readHeader failed - " << e.what() << std::endl;
        throw; // 重新抛出，让上层测试捕获
    }
    std::cout << "DiskManager constructor: readHeader completed." << std::endl;
    std::cout << "Page count: " << page_count_ << ", Free list head: " << free_list_head_ << std::endl;
}

DiskManager::~DiskManager() {
    try {
        writeHeader(); // 析构函数中捕获异常，避免程序崩溃
    } catch (const DiskException& e) {
        std::cerr << "DiskManager destructor: writeHeader failed - " << e.what() << std::endl;
    }
}



void DiskManager::readPage(PageID page_id, char* data) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    auto& file_stream = file_manager_->getFileStream();

    // 1. 检查流状态
    if (!file_stream.good()) {
        throw DiskException("File stream invalid for readPage (page=" + std::to_string(page_id) + ")");
    }

    // 2. 检查页ID合法性
    if (page_id < 0 || page_id >= page_count_) {
        throw PageOutOfRangeException(page_id); // 明确抛出页超出范围异常
    }

    // 3. 计算偏移并检查文件大小
    size_t offset = FILE_HEADER_SIZE + static_cast<size_t>(page_id) * PAGE_SIZE;
    file_stream.seekg(0, std::ios::end);
    size_t file_size = static_cast<size_t>(file_stream.tellg());
    if (offset + PAGE_SIZE > file_size) {
        throw DiskException("Page " + std::to_string(page_id) + " out of file size (file=" +
                          std::to_string(file_size) + " bytes, required=" + std::to_string(offset + PAGE_SIZE) + ")");
    }

    // 4. 读取页面数据
    file_stream.seekg(offset);
    file_stream.read(data, PAGE_SIZE);
    if (file_stream.gcount() != PAGE_SIZE) {
        throw IOException("Read page " + std::to_string(page_id) + " failed: read " +
                        std::to_string(file_stream.gcount()) + " bytes (expected " + std::to_string(PAGE_SIZE) + ")");
    }

    std::cout << "readPage: Success - PageID=" << page_id << ", Offset=" << offset << std::endl;
}

void DiskManager::writePage(PageID page_id, const char* data) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    auto& file_stream = file_manager_->getFileStream();

    std::cout << "writePage: Starting - PageID=" << page_id << std::endl;

    // 1. 检查流状态
    if (!file_stream.good()) {
        std::cout << "writePage: File stream not good" << std::endl;
        throw DiskException("File stream invalid for writePage (page=" + std::to_string(page_id) + ")");
    }

    // 2. 计算目标偏移
    size_t offset = FILE_HEADER_SIZE + static_cast<size_t>(page_id) * PAGE_SIZE;
    std::cout << "writePage: Target Offset=" << offset << std::endl;

    // 3. 检查文件大小并扩展（如果需要）
    file_stream.seekp(0, std::ios::end);
    size_t current_file_size = static_cast<size_t>(file_stream.tellp());
    size_t required_size = offset + PAGE_SIZE;
    std::cout << "writePage: Current File Size=" << current_file_size << ", Required Size=" << required_size << std::endl;

    if (required_size > current_file_size) {
        std::cout << "writePage: Extending file" << std::endl;
        // 扩展文件
        file_stream.seekp(required_size - 1);
        file_stream.put('\0');
        file_stream.flush();

        if (file_stream.fail()) {
            std::cout << "writePage: File extension failed" << std::endl;
            throw IOException("Extend file failed for page " + std::to_string(page_id) + " (disk full?)");
        }
        std::cout << "writePage: File extended successfully" << std::endl;
    }

    // 4. 定位到目标偏移并写入数据
    std::cout << "writePage: Seeking to offset " << offset << std::endl;
    file_stream.seekp(offset);
    if (file_stream.fail()) {
        std::cout << "writePage: Seek failed" << std::endl;
        throw IOException("Seek to offset " + std::to_string(offset) + " failed for page " + std::to_string(page_id));
    }

    std::cout << "writePage: Writing data" << std::endl;
    file_stream.write(data, PAGE_SIZE);
    file_stream.flush();

    // 5. 验证写入结果
    if (file_stream.fail()) {
        std::cout << "writePage: Write failed" << std::endl;
        throw IOException("Write page " + std::to_string(page_id) + " failed (stream error)");
    }
    std::cout << "writePage: Success - PageID=" << page_id << ", Written " << PAGE_SIZE << " bytes" << std::endl;
}

PageID DiskManager::allocatePage() {
    std::lock_guard<std::mutex> lock(io_mutex_);
    std::cout << "allocatePage: Starting - Current PageCount=" << page_count_ << ", FreeListHead=" << free_list_head_ << std::endl;

    PageID allocated_page_id = INVALID_PAGE_ID;

    if (free_list_head_ != INVALID_PAGE_ID) {
        // 补全空闲链表分配逻辑
        allocated_page_id = free_list_head_;
        std::cout << "allocatePage: Reusing free page - PageID=" << allocated_page_id << std::endl;

        // 读取空闲页的 next_free_page，更新空闲链表头
        char buffer[PAGE_SIZE];
        readPage(allocated_page_id, buffer); // 复用 readPage 逻辑，避免重复代码
        PageHeader* free_page_header = reinterpret_cast<PageHeader*>(buffer);

        // 验证空闲页合法性
        if (free_page_header->page_type != PageType::FREE_PAGE) {
            throw DiskException("Invalid free page: PageID=" + std::to_string(allocated_page_id) + " (not marked as FREE_PAGE)");
        }
        free_list_head_ = free_page_header->next_free_page; // 更新链表头
        std::cout << "allocatePage: Updated FreeListHead to " << free_list_head_ << std::endl;

    } else {
        // 分配新页（原逻辑修复）
        allocated_page_id = page_count_;
        // 先增加页计数
        page_count_++;
        std::cout << "allocatePage: Updated PageCount to " << page_count_ << std::endl;

        // 然后写入页面数据
        char buffer[PAGE_SIZE] = {0};
        PageHeader* new_page_header = reinterpret_cast<PageHeader*>(buffer);
        new_page_header->page_id = allocated_page_id;
        new_page_header->page_type = PageType::DATA_PAGE;
        new_page_header->next_free_page = INVALID_PAGE_ID;
        new_page_header->free_space_offset = 0;
        new_page_header->slot_count = 0;
        new_page_header->free_space = PAGE_SIZE - sizeof(PageHeader);
        new_page_header->is_dirty = false;

        try {
            writePage(allocated_page_id, buffer);
        } catch (const DiskException& e) {
            // 如果写入失败，回滚页计数
            page_count_--;
            throw DiskException("Allocate new page failed: " + std::string(e.what()));
        }
    }

    // 持久化文件头
    writeHeader();
    std::cout << "allocatePage: Succeeded - Allocated PageID=" << allocated_page_id << std::endl;

    return allocated_page_id;

}

void DiskManager::deallocatePage(PageID page_id) {
    std::lock_guard<std::mutex> lock(io_mutex_);

    // 1. 检查页ID合法性
    if (page_id == INVALID_PAGE_ID || page_id >= page_count_) {
        throw PageOutOfRangeException(page_id);
    }
    // 2. 检查是否重复释放
    if (page_id == free_list_head_) {
        throw DiskException("Page " + std::to_string(page_id) + " is already in free list");
    }

    // 3. 初始化空闲页头（修复：对齐方式与 Page 类一致）
    char buffer[PAGE_SIZE] = {0};
    PageHeader* free_page_header = reinterpret_cast<PageHeader*>(buffer);
    free_page_header->page_id = page_id;
    free_page_header->page_type = PageType::FREE_PAGE;
    free_page_header->next_free_page = free_list_head_; // 插入链表头部
    free_page_header->free_space = PAGE_SIZE - sizeof(PageHeader); // 重置空闲空间

    // 4. 写入空闲页并更新链表
    writePage(page_id, buffer);
    free_list_head_ = page_id;
    writeHeader(); // 持久化元数据

    std::cout << "deallocatePage: Success - PageID=" << page_id << ", New FreeListHead=" << free_list_head_ << std::endl;
}

void DiskManager::flush() {
    std::lock_guard<std::mutex> lock(io_mutex_);
    std::cout << "flush: Starting - Persisting metadata" << std::endl;
    writeHeader(); // 刷元数据
    auto& file_stream = file_manager_->getFileStream();
    file_stream.flush(); // 刷文件流
    if (file_stream.fail()) {
        throw IOException("Flush file stream failed");
    }
    std::cout << "flush: Success" << std::endl;
}

PageID DiskManager::getPageCount() const {
    std::lock_guard<std::mutex> lock(io_mutex_);
    return page_count_;
}

void DiskManager::readHeader() {
    std::lock_guard<std::mutex> lock(io_mutex_);
    std::cout << "readHeader: Starting..." << std::endl;

    if (!file_manager_->isOpen()) {
        throw DiskException("Database not open in readHeader");
    }

    auto& file_stream = file_manager_->getFileStream();
    std::streampos original_pos = file_stream.tellg();
    PageID temp_page_count = 0;
    PageID temp_free_head = 0;

    try {
        // 读取文件头（8字节：4字节页计数 + 4字节空闲链表头）
        file_stream.seekg(0, std::ios::beg);
        file_stream.read(reinterpret_cast<char*>(&temp_page_count), sizeof(PageID));
        size_t bytes_read1 = file_stream.gcount();
        file_stream.read(reinterpret_cast<char*>(&temp_free_head), sizeof(PageID));
        size_t bytes_read2 = file_stream.gcount();

        std::cout << "readHeader: Raw - PageCount=" << temp_page_count << " (" << bytes_read1 << " bytes), FreeListHead=" << temp_free_head << " (" << bytes_read2 << " bytes)" << std::endl;

        // 校正无效值（确保元数据合法）
        if (temp_page_count < 1 || bytes_read1 != sizeof(PageID)) {
            temp_page_count = 1;
            std::cout << "readHeader: Corrected PageCount to 1 (invalid raw value)" << std::endl;
        }
        if (temp_free_head < INVALID_PAGE_ID || temp_free_head >= temp_page_count || bytes_read2 != sizeof(PageID)) {
            temp_free_head = INVALID_PAGE_ID;
            std::cout << "readHeader: Corrected FreeListHead to -1 (invalid raw value)" << std::endl;
        }

        // 检查文件大小（确保至少能容纳0号页）
        file_stream.seekg(0, std::ios::end);
        size_t file_size = static_cast<size_t>(file_stream.tellg());
        size_t min_required_size = FILE_HEADER_SIZE + PAGE_SIZE; // 0号页的大小
        if (file_size < min_required_size) {
            std::cout << "readHeader: File too small (" << file_size << " bytes), extending to " << min_required_size << " bytes" << std::endl;
            // 扩展文件以容纳0号页
            file_stream.seekp(min_required_size - 1);
            file_stream.put('\0');
            file_stream.flush();
        }

        // 更新成员变量
        page_count_ = temp_page_count;
        free_list_head_ = temp_free_head;
        std::cout << "readHeader: Final - PageCount=" << page_count_ << ", FreeListHead=" << free_list_head_ << std::endl;

    } catch (const std::exception& e) {
        throw DiskException("readHeader failed: " + std::string(e.what()));
    }

        // 恢复流位置，避免影响后续操作
        file_stream.seekg(original_pos);
        // 清除流错误状态
        if (file_stream.fail())
            {
            file_stream.clear();
            std::cout << "readHeader: Cleared stream error state" << std::endl;
        }

}

void DiskManager::writeHeader() {
    std::lock_guard<std::mutex> lock(io_mutex_);
    std::cout << "writeHeader: Writing - PageCount=" << page_count_ << ", FreeListHead=" << free_list_head_ << std::endl;

    if (!file_manager_->isOpen()) {
        throw DiskException("Database not open in writeHeader");
    }

    auto& file_stream = file_manager_->getFileStream();
    std::streampos original_pos = file_stream.tellp();

    try {
        // 定位到文件头并写入
        file_stream.seekp(0, std::ios::beg);
        file_stream.write(reinterpret_cast<const char*>(&page_count_), sizeof(PageID));
        if (file_stream.fail()) {
            throw IOException("Write PageCount failed in writeHeader");
        }
        file_stream.write(reinterpret_cast<const char*>(&free_list_head_), sizeof(PageID));
        if (file_stream.fail()) {
            throw IOException("Write FreeListHead failed in writeHeader");
        }
        file_stream.flush();
        std::cout << "writeHeader: Success" << std::endl;

    } catch (const std::exception& e) {
        throw DiskException("writeHeader failed: " + std::string(e.what()));
    }
        // 恢复流位置
        file_stream.seekp(original_pos);

}

} // namespace storage
} // namespace minidb