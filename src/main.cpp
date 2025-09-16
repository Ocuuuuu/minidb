#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "storage/BufferManager.h"
#include "storage/DiskManager.h"
#include  "storage/FileManager.h"
#include "storage/Page.h"

// 辅助函数：打印页面摘要
void printPageInfo(minidb::storage::Page* page, const std::string& when) {
    auto& header = page->getHeader();
    std::cout << "\n=== Page Info - " << when << " ===" << std::endl;
    std::cout << "Page ID: " << header.page_id << std::endl;
    std::cout << "Slot Count: " << header.slot_count << std::endl;
    std::cout << "Free Space Offset: " << header.free_space_offset << std::endl;
    std::cout << "Free Space: " << header.free_space << std::endl;
    std::cout << "Is Dirty: " << (header.is_dirty ? "Yes" : "No") << std::endl;

    for (uint16_t i = 0; i < header.slot_count; ++i) {
        uint16_t offset, size;
        bool ok = page->getSlotInfo(i, &offset, &size);
        if (!ok) {
            std::cout << "Slot " << i << ": INVALID" << std::endl;
            continue;
        }

        std::string preview;
        int preview_len = std::min(static_cast<int>(size), 20);
        for (int j = 0; j < preview_len; ++j) {
            char c = page->getData()[offset + j];
            if (c >= 32 && c <= 126) {
                preview += c;
            } else {
                preview += '?';
            }
        }

        std::cout << "Slot " << i
                  << " -> offset=" << std::setw(4) << offset
                  << ", size=" << std::setw(3) << size
                  << ", data='" << preview << "'"
                  << std::endl;

        // 添加十六进制转储用于调试
        std::cout << "        Hex: ";
        for (int j = 0; j < std::min(10, static_cast<int>(size)); ++j) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << (int)(unsigned char)page->getData()[offset + j] << " ";
        }
        std::cout << std::dec << std::endl;
    }
    std::cout << "=== End Page Info ===\n" << std::endl;
}

// 辅助函数：转储原始页面内容
void dumpRawPageContent(const char* page_data, const std::string& title) {
    std::cout << "\n=== Raw Page Dump - " << title << " ===" << std::endl;
    for (int i = 0; i < 100; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)(unsigned char)page_data[i] << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
    std::cout << "=== End Raw Dump ===\n" << std::endl;
}

int main() {
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_debug_db";

    // 清理之前的测试数据库
    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    const char* test_data = "PersistentTestData123";
    uint16_t data_len = std::strlen(test_data) + 1;
    minidb::RID stored_rid;
    minidb::PageID page_id;

    try {
        // ==================== Phase 1: Write ====================
        {
            std::cout << "=== Phase 1: Writing data to disk ===" << std::endl;

            file_manager->createDatabase(test_db);
            auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
            minidb::storage::BufferManager buffer_manager(disk_manager, 10);

            page_id = disk_manager->allocatePage();
            minidb::storage::Page* page = buffer_manager.fetchPage(page_id);

            if (page == nullptr) {
                std::cerr << "? Failed to fetch page!" << std::endl;
                return 1;
            }

            std::cout << "Before insert:" << std::endl;
            printPageInfo(page, "Before Insert");
            dumpRawPageContent(page->getData(), "Before Insert");

            bool inserted = page->insertRecord(test_data, data_len, &stored_rid);
            if (!inserted) {
                std::cerr << "? Failed to insert record!" << std::endl;
                return 1;
            }

            std::cout << "? Inserted record: '" << test_data << "' (length: " << data_len << ")" << std::endl;

            std::cout << "After insert:" << std::endl;
            printPageInfo(page, "After Insert");
            dumpRawPageContent(page->getData(), "After Insert");

            // 验证插入的数据
            char read_data[100];
            uint16_t read_size;
            bool found = page->getRecord(stored_rid, read_data, &read_size);

            std::cout << "Self-check - Found: " << found << ", Size: " << read_size << std::endl;
            if (found) {
                std::cout << "Self-check - Data: '" << read_data << "'" << std::endl;
                std::cout << "Self-check - Hex: ";
                for (int i = 0; i < std::min(20, static_cast<int>(read_size)); ++i) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << (int)(unsigned char)read_data[i] << " ";
                }
                std::cout << std::dec << std::endl;
            }

            if (!found || read_size != data_len || std::strcmp(read_data, test_data) != 0) {
                std::cerr << "? Self-check failed!" << std::endl;
                return 1;
            }

            buffer_manager.unpinPage(page_id, true);
            std::cout << "Flushing all pages..." << std::endl;
            buffer_manager.flushAllPages();
            std::cout << "All pages flushed." << std::endl;
        }

        std::cout << "Phase 1 completed. Simulating restart..." << std::endl;

        // 直接从磁盘文件读取验证
        std::ifstream file(test_db + ".db", std::ios::binary);
        if (file) {
            file.seekg(page_id * 4096);
            char disk_page[4096];
            file.read(disk_page, 4096);

            std::cout << "Direct disk read after flush:" << std::endl;
            dumpRawPageContent(disk_page, "Direct Disk Read");

            // 检查是否包含我们的测试数据
            const char* search_str = test_data;
            bool found_in_disk = false;
            for (int i = 0; i < 4096 - data_len; i++) {
                if (std::memcmp(disk_page + i, search_str, data_len) == 0) {
                    found_in_disk = true;
                    std::cout << "? Found test data at offset: " << i << std::endl;
                    break;
                }
            }

            if (!found_in_disk) {
                std::cout << "? Test data not found in raw disk content!" << std::endl;
            }
        }

        // ==================== Phase 2: Read after restart ====================
        {
            std::cout << "=== Phase 2: Reading after restart ===" << std::endl;

            auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
            minidb::storage::BufferManager buffer_manager(disk_manager, 10);

            minidb::storage::Page* page = buffer_manager.fetchPage(page_id);
            if (page == nullptr) {
                std::cerr << "? Failed to fetch page after restart!" << std::endl;
                return 1;
            }

            std::cout << "After restart, page info:" << std::endl;
            printPageInfo(page, "After Restart");
            dumpRawPageContent(page->getData(), "After Restart");

            char read_data[100];
            uint16_t read_size;
            bool found = page->getRecord(stored_rid, read_data, &read_size);

            std::cout << "Restart read result: " << (found ? "Success" : "Failed") << std::endl;
            if (found) {
                std::cout << "Read data: '" << read_data << "' (length: " << read_size << ")" << std::endl;
                std::cout << "Hex: ";
                for (int i = 0; i < std::min(20, static_cast<int>(read_size)); ++i) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << (int)(unsigned char)read_data[i] << " ";
                }
                std::cout << std::dec << std::endl;
            }

            if (!found) {
                std::cerr << "? Failed to read data after restart!" << std::endl;
                return 1;
            }

            if (read_size != data_len) {
                std::cerr << "? Size mismatch after restart! Expected: " << data_len
                          << ", Got: " << read_size << std::endl;
                return 1;
            }

            if (std::strcmp(read_data, test_data) != 0) {
                std::cerr << "? Data corruption after restart!" << std::endl;
                std::cerr << "Expected: '" << test_data << "'" << std::endl;
                std::cerr << "Got:      '" << read_data << "'" << std::endl;

                // 输出十六进制对比
                std::cout << "Expected hex: ";
                for (int i = 0; i < data_len; ++i) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << (int)(unsigned char)test_data[i] << " ";
                }
                std::cout << std::endl << "Actual hex:   ";
                for (int i = 0; i < read_size; ++i) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << (int)(unsigned char)read_data[i] << " ";
                }
                std::cout << std::dec << std::endl;

                return 1;
            }

            std::cout << "? Persistence Test PASSED! Data survived restart." << std::endl;
            buffer_manager.unpinPage(page_id, false);
        }

        // 清理
        if (file_manager->databaseExists(test_db)) {
            file_manager->deleteDatabase(test_db);
        }

        std::cout << "? All tests passed successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "? Exception occurred: " << e.what() << std::endl;

        if (file_manager->databaseExists(test_db)) {
            file_manager->deleteDatabase(test_db);
        }
        
        return 1;
    }
}