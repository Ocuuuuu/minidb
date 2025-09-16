// src/storage/BufferManager.cpp

#include "../../include/storage/BufferManager.h"
#include "../../include/storage/DiskManager.h"
#include "../../include/common/Constants.h"
#include "../../include/common/Exception.h"
#include <iostream>
#include <cstring>
#include <iomanip>


namespace minidb {
namespace storage {

// ====================== 构造与析构 ======================
BufferManager::BufferManager(std::shared_ptr<DiskManager> disk_manager,
                             size_t pool_size)
    : disk_manager_(std::move(disk_manager)),
      pool_size_(pool_size),
      hit_count_(0),
      miss_count_(0) {
    if (pool_size_ == 0) {
        throw BufferPoolException("Buffer pool size cannot be zero");
    }
}

BufferManager::~BufferManager() {
    flushAllPages();
}

// ====================== 核心方法：获取页面 ======================
Page* BufferManager::fetchPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    // 检查页面是否已在缓冲池
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        updateAccessTime(page_id);
        it->second.pin_count++;
        hit_count_++;
        return &(it->second.page);
    }

    // 页面未命中，从磁盘加载
    miss_count_++;

    // 缓冲池已满时淘汰页面
    if (page_table_.size() >= pool_size_) {
        if (!evictPage()) {
            throw BufferPoolFullException("Cannot evict any page (all pinned)");
        }
    }

    // 从磁盘读取页面数据
    char buffer[PAGE_SIZE];
    disk_manager_->readPage(page_id, buffer);

    // 创建缓冲帧并反序列化
    BufferFrame frame;
    frame.page.deserialize(buffer);

    // ✅ 修复：正确判断是否为"新页面"
    auto& header = frame.page.getHeader();
    bool is_new_page = false;

    // 更准确的新页面判断逻辑：
    // 1. 检查页面ID是否有效
    // 2. 检查页面是否被正确初始化过
    if (header.page_id == INVALID_PAGE_ID ||
        (header.page_id == 0 && page_id != 0) || // page_id=0是有效的
        header.free_space_offset > PAGE_SIZE ||  // 无效的偏移量
        header.free_space > PAGE_SIZE) {         // 无效的空闲空间
        is_new_page = true;

        std::cout << "DEBUG: Detected uninitialized page " << page_id
                  << ", initializing..." << std::endl;
    }

    if (is_new_page) {
        // ✅ 正确初始化页面
        header.page_id = page_id;
        header.page_type = PageType::DATA_PAGE;
        header.slot_count = 0;
        header.free_space_offset = PAGE_SIZE;  // 从高地址开始写入
        header.free_space = PAGE_SIZE - sizeof(PageHeader);
        header.is_dirty = true;
        header.next_free_page = INVALID_PAGE_ID;

        std::cout << "DEBUG: Page " << page_id << " initialized - "
                  << "free_space: " << header.free_space
                  << ", free_space_offset: " << header.free_space_offset << std::endl;

        // 立即序列化回页面数据
        frame.page.serialize(buffer);
    } else {
        // ✅ 安全检查：确保 page_id 一致
        if (header.page_id != page_id) {
            std::cerr << "WARNING: Page ID mismatch: expected " << page_id
                      << ", but found " << header.page_id << ". Correcting."
                      << std::endl;
            header.page_id = page_id;
            header.is_dirty = true;

            // 更新序列化数据
            frame.page.serialize(buffer);
        }
    }

    frame.pin_count = 1;
    frame.is_dirty = header.is_dirty;

    // 加入LRU链表头部
    lru_list_.push_front(page_id);
    frame.iterator = lru_list_.begin();

    // 加入页表映射
    auto [inserted_it, success] = page_table_.emplace(page_id, std::move(frame));
    if (!success) {
        throw BufferPoolException("Failed to add page to buffer pool");
    }

    return &(inserted_it->second.page);
}

// ====================== 页面固定与解锁 ======================
    void BufferManager::pinPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    it->second.pin_count++;
}

    void BufferManager::unpinPage(PageID page_id, bool is_dirty) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    if (it->second.pin_count == 0) {
        throw BufferPoolException("Cannot unpin page " + std::to_string(page_id) + " (pin count is zero)");
    }
    it->second.pin_count--;

    if (is_dirty) {
        it->second.is_dirty = true;
        it->second.page.setDirty(true);
    }
}


void BufferManager::flushPage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        std::cout << "❌ Page " << page_id << " not found in buffer pool" << std::endl;
        throw PageNotInPoolException(page_id);
    }

    bool is_dirty = it->second.is_dirty || it->second.page.isDirty();

    if (is_dirty) {
        std::cout << "DEBUG: Flushing page " << page_id << std::endl;

        Page& page = it->second.page;
        const PageHeader& header = page.getHeader();

        std::cout << "DEBUG: sizeof(PageHeader) = " << sizeof(PageHeader) << " bytes" << std::endl;
        std::cout << "DEBUG: Page header - page_id: " << header.page_id
                 << ", slot_count: " << header.slot_count
                 << ", free_space_offset: " << header.free_space_offset
                 << ", free_space: " << header.free_space << std::endl;

        // 使用标准序列化
        char page_buffer[PAGE_SIZE];
        page.serialize(page_buffer);

        // ✅ 添加详细的内存内容检查
        std::cout << "DEBUG: === FULL PAGE MEMORY ANALYSIS ===" << std::endl;

        // 检查整个页面中是否有完整的数据
        const char* search_str = "PersistentTestData123";
        bool found_in_memory = false;

        for (int i = sizeof(PageHeader); i < PAGE_SIZE - strlen(search_str); i++) {
            if (memcmp(page_buffer + i, search_str, strlen(search_str)) == 0) {
                std::cout << "✅ Found complete string at memory offset: " << i
                          << " (file offset: " << (page_id * PAGE_SIZE + i) << ")" << std::endl;
                found_in_memory = true;
                break;
            }
        }

        if (!found_in_memory) {
            std::cout << "❌ Complete string NOT found in memory!" << std::endl;

            // 输出页面中所有非零字节
            std::cout << "DEBUG: Non-zero bytes in page:" << std::endl;
            for (int i = 0; i < PAGE_SIZE; i++) {
                if (page_buffer[i] != 0) {
                    std::cout << "Offset " << i << ": " << std::hex << (int)(unsigned char)page_buffer[i]
                              << std::dec;
                    if (page_buffer[i] >= 32 && page_buffer[i] <= 126) {
                        std::cout << " ('" << page_buffer[i] << "')";
                    }
                    std::cout << std::endl;
                }
            }
        }

        // 正确的偏移计算
        uint16_t data_offset_in_page = header.free_space_offset;
        uint32_t data_offset_in_file = page_id * PAGE_SIZE + sizeof(PageHeader) + data_offset_in_page;

        std::cout << "DEBUG: Data should be at offset: " << data_offset_in_file
                  << " in file (page offset: " << data_offset_in_page << ")" << std::endl;
        std::cout << "DEBUG: Actual data at offset " << data_offset_in_file << ": ";

        // 使用序列化缓冲区的数据
        for (int i = sizeof(PageHeader) + data_offset_in_page;
             i < sizeof(PageHeader) + data_offset_in_page + 22 && i < PAGE_SIZE; i++) {
            printf("%02x ", (unsigned char)page_buffer[i]);
        }
        printf("\n");

        // 检查内存中的数据
        if (sizeof(PageHeader) + data_offset_in_page + 22 <= PAGE_SIZE) {
            std::string memory_data(page_buffer + sizeof(PageHeader) + data_offset_in_page, 22);
            std::cout << "DEBUG: Memory data as string: '" << memory_data << "'" << std::endl;

            if (memory_data == "PersistentTestData123") {
                std::cout << "✅ Data correct in memory" << std::endl;
            } else {
                std::cout << "❌ Data corrupted in memory! Expected 22 chars, got: "
                          << memory_data.length() << " chars" << std::endl;
            }
        }

        // 写入磁盘
        try {
            disk_manager_->writePage(page_id, page_buffer);
            std::cout << "✅ Page " << page_id << " written to disk" << std::endl;

            // 立即验证磁盘写入
            char disk_buffer[PAGE_SIZE];
            disk_manager_->readPage(page_id, disk_buffer);

            // 检查磁盘数据
            PageHeader disk_header;
            std::memcpy(&disk_header, disk_buffer, sizeof(PageHeader));

            uint16_t disk_data_offset_in_page = disk_header.free_space_offset;
            uint32_t disk_data_offset_in_file = page_id * PAGE_SIZE + sizeof(PageHeader) + disk_data_offset_in_page;

            std::cout << "DEBUG: Disk data at offset " << disk_data_offset_in_file << ": ";
            for (int i = sizeof(PageHeader) + disk_data_offset_in_page;
                 i < sizeof(PageHeader) + disk_data_offset_in_page + 22 && i < PAGE_SIZE; i++) {
                printf("%02x ", (unsigned char)disk_buffer[i]);
            }
            printf("\n");

            // 转换为字符串检查磁盘数据
            if (sizeof(PageHeader) + disk_data_offset_in_page + 22 <= PAGE_SIZE) {
                std::string disk_data(disk_buffer + sizeof(PageHeader) + disk_data_offset_in_page, 22);
                std::cout << "DEBUG: Disk data as string: '" << disk_data << "'" << std::endl;

                if (disk_data == "PersistentTestData123") {
                    std::cout << "✅ Data verified on disk" << std::endl;
                } else {
                    std::cout << "❌ Data mismatch on disk" << std::endl;

                    // 比较内存和磁盘数据
                    bool match = true;
                    for (int i = 0; i < PAGE_SIZE; i++) {
                        if (page_buffer[i] != disk_buffer[i]) {
                            std::cout << "Mismatch at offset " << i << ": memory="
                                      << std::hex << (int)(unsigned char)page_buffer[i]
                                      << " disk=" << std::hex << (int)(unsigned char)disk_buffer[i]
                                      << std::dec << std::endl;
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        std::cout << "✅ Memory and disk data match" << std::endl;
                    }
                }
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Failed to write page " << page_id << ": " << e.what() << std::endl;
            return;
        }

        it->second.is_dirty = false;
        page.setDirty(false);

        std::cout << "✅ Page " << page_id << " flushed successfully" << std::endl;
    } else {
        std::cout << "Page " << page_id << " is not dirty, skipping flush" << std::endl;
    }
}

// 辅助函数：验证磁盘写入
void verifyDiskWrite(DiskManager* disk_manager, PageID page_id, const char* expected_data) {
    try {
        char read_buffer[PAGE_SIZE];
        disk_manager->readPage(page_id, read_buffer);

        std::cout << "DEBUG: Verifying disk write for page " << page_id << std::endl;

        // 检查是否包含预期数据
        bool found_on_disk = false;
        for (int i = 0; i < PAGE_SIZE - strlen(expected_data); i++) {
            if (std::memcmp(read_buffer + i, expected_data, strlen(expected_data)) == 0) {
                found_on_disk = true;
                std::cout << "✅ Test data found on disk at offset: " << i << std::endl;
                break;
            }
        }

        if (!found_on_disk) {
            std::cout << "❌ Test data NOT found on disk after write!" << std::endl;

            // 输出磁盘读取的内容
            std::cout << "Disk content (first 100 bytes):" << std::endl;
            for (int i = 0; i < 100; i++) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << (int)(unsigned char)read_buffer[i] << " ";
                if ((i + 1) % 16 == 0) std::cout << std::endl;
            }
            std::cout << std::dec << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "❌ Disk verification failed: " << e.what() << std::endl;
    }
}


    void BufferManager::flushAllPages() {
    std::cout << "flushAllPages() called - checking " << page_table_.size() << " pages" << std::endl;

    int flushed_count = 0;
    for (auto& [page_id, frame] : page_table_) {
        bool is_dirty = frame.is_dirty || frame.page.isDirty();

        std::cout << "Page " << page_id << ": dirty=" << is_dirty
                  << ", frame.dirty=" << frame.is_dirty
                  << ", page.dirty=" << frame.page.isDirty() << std::endl;

        if (is_dirty) {
            std::cout << "Flushing page " << page_id << " (dirty)" << std::endl;
            flushPage(page_id);
            flushed_count++;
        } else {
            std::cout << "Skipping page " << page_id << " (not dirty)" << std::endl;
        }
    }
    std::cout << "Total pages flushed: " << flushed_count << std::endl;
}

// ====================== 页面移除 ======================
void BufferManager::removePage(PageID page_id) {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    if (it->second.pin_count > 0) {
        throw PinnedPageException(page_id);
    }

    if (it->second.is_dirty) {
        char buffer[PAGE_SIZE];
        it->second.page.serialize(buffer);
        disk_manager_->writePage(page_id, buffer);
    }

    lru_list_.erase(it->second.iterator);
    page_table_.erase(it);
}

// ====================== LRU淘汰策略 ======================
bool BufferManager::evictPage() {
    auto list_it = lru_list_.rbegin();
    while (list_it != lru_list_.rend()) {
        PageID evict_candidate = *list_it;
        auto table_it = page_table_.find(evict_candidate);

        if (table_it == page_table_.end()) {
            auto forward_it = --(list_it.base());
            list_it = std::make_reverse_iterator(lru_list_.erase(forward_it));
            continue;
        }

        BufferFrame& frame = table_it->second;

        if (frame.pin_count > 0) {
            ++list_it;
            continue;
        }

        if (frame.is_dirty) {
            char buffer[PAGE_SIZE];
            frame.page.serialize(buffer);
            disk_manager_->writePage(evict_candidate, buffer);
        }

        lru_list_.erase(frame.iterator);
        page_table_.erase(table_it);
        return true;
    }

    return false;
}

// ====================== 更新访问时间 ======================
void BufferManager::updateAccessTime(PageID page_id) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        throw PageNotInPoolException(page_id);
    }

    lru_list_.erase(it->second.iterator);
    lru_list_.push_front(page_id);
    it->second.iterator = lru_list_.begin();
}

// ====================== 命中率计算 ======================
double BufferManager::getHitRate() const {
    std::shared_lock<std::shared_mutex> lock(buffer_mutex_);
    size_t total = hit_count_ + miss_count_;
    return total == 0 ? 0.0 : static_cast<double>(hit_count_) / total;
}

} // namespace storage
} // namespace minidb