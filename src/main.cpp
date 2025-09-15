#include <iostream>
#include <cstring>
#include <memory>
#include <vector>
#include <filesystem>
#include "common/Types.h"
#include "storage/DiskManager.h"
#include "storage/FileManager.h"

// 函数声明
void testPageAllocation(minidb::storage::DiskManager& diskManager);
void testPageReadWrite(minidb::storage::DiskManager& diskManager);
void testPageDeallocation(minidb::storage::DiskManager& diskManager);
void testDiskManager();
void diskManagerTestMenu();
void cleanupDatabase();

namespace fs = std::filesystem;

// 清理数据库文件
void cleanupDatabase() {
    std::string dbPath = "test_db.minidb";
    if (fs::exists(dbPath)) {
        try {
            std::cout << "Removing existing database file: " << dbPath << std::endl;
            fs::remove(dbPath);
            std::cout << "Database file removed successfully" << std::endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to remove database file: " << e.what() << std::endl;
        }
    }
}

// 创建并配置 FileManager
std::shared_ptr<minidb::storage::FileManager> createFileManager() {
    auto fileManager = std::make_shared<minidb::storage::FileManager>();

    // 检查数据库是否存在，如果不存在则创建
    std::string dbName = "test_db";

    // 先检查文件状态
    std::string dbPath = dbName + ".minidb";
    if (fs::exists(dbPath)) {
        std::cout << "Database file exists, size: "
                  << fs::file_size(dbPath) << " bytes" << std::endl;
    } else {
        std::cout << "Database file does not exist" << std::endl;
    }

    if (fileManager->databaseExists(dbName)) {
        std::cout << "Opening existing database: " << dbName << std::endl;
        fileManager->openDatabase(dbName);
    } else {
        std::cout << "Creating new database: " << dbName << std::endl;
        fileManager->createDatabase(dbName);
    }

    return fileManager;
}

// 测试页面分配
void testPageAllocation(minidb::storage::DiskManager& diskManager) {
    std::cout << "\n--- Testing Page Allocation ---" << std::endl;
    std::cout << "Initial page count: " << diskManager.getPageCount() << std::endl;
    std::cout << "Free list head: " << diskManager.getFreeListHead() << std::endl;

    for (int i = 0; i < 3; ++i) {
        std::cout << "Allocating page " << i << "..." << std::endl;
        auto page_id = diskManager.allocatePage();
        std::cout << "Allocated page " << i << ": " << page_id << std::endl;
    }

    std::cout << "Final page count: " << diskManager.getPageCount() << std::endl;
    std::cout << "Free list head: " << diskManager.getFreeListHead() << std::endl;
}

// 测试页面读写
void testPageReadWrite(minidb::storage::DiskManager& diskManager) {
    std::cout << "\n--- Testing Page Read/Write ---" << std::endl;

    auto page_id = diskManager.allocatePage();
    std::cout << "Using page: " << page_id << std::endl;

    // 写入数据
    char write_data[minidb::PAGE_SIZE];
    std::memset(write_data, 0, minidb::PAGE_SIZE);
    std::string test_message = "This is a test message for page read/write!";
    std::strncpy(write_data, test_message.c_str(), test_message.size());

    std::cout << "Writing to page..." << std::endl;
    diskManager.writePage(page_id, write_data);
    std::cout << "Written: " << test_message << std::endl;

    // 读取数据
    char read_data[minidb::PAGE_SIZE];
    std::memset(read_data, 0, minidb::PAGE_SIZE);

    std::cout << "Reading from page..." << std::endl;
    diskManager.readPage(page_id, read_data);
    std::cout << "Read: " << read_data << std::endl;

    // 验证数据一致性
    if (std::strcmp(write_data, read_data) == 0) {
        std::cout << "Data consistency verified!" << std::endl;
    } else {
        std::cout << "Data consistency check failed!" << std::endl;
    }
}

// 测试页面释放
void testPageDeallocation(minidb::storage::DiskManager& diskManager) {
    std::cout << "\n--- Testing Page Deallocation ---" << std::endl;

    // 分配几个页面
    std::vector<minidb::PageID> pages;
    for (int i = 0; i < 3; ++i) {
        pages.push_back(diskManager.allocatePage());
        std::cout << "Allocated page: " << pages.back() << std::endl;
    }

    std::cout << "Page count before deallocation: " << diskManager.getPageCount() << std::endl;
    std::cout << "Free list head: " << diskManager.getFreeListHead() << std::endl;

    // 释放中间页面
    std::cout << "Deallocating page " << pages[1] << "..." << std::endl;
    diskManager.deallocatePage(pages[1]);
    std::cout << "Deallocated page: " << pages[1] << std::endl;
    std::cout << "Free list head after deallocation: " << diskManager.getFreeListHead() << std::endl;

    // 尝试重新分配（应该得到释放的页面）
    std::cout << "Reallocating page..." << std::endl;
    auto new_page = diskManager.allocatePage();
    std::cout << "Reallocated page (should be " << pages[1] << "): " << new_page << std::endl;
    std::cout << "Free list head after reallocation: " << diskManager.getFreeListHead() << std::endl;

    std::cout << "Page count after operations: " << diskManager.getPageCount() << std::endl;
}

// 测试 DiskManager 的功能
void testDiskManager() {
    std::cout << "\n=== Testing DiskManager ===" << std::endl;

    try {
        // 创建文件管理器
        auto fileManager = createFileManager();
        std::cout << "FileManager created successfully" << std::endl;
        // 检查文件流状态
        auto& file = fileManager->getFileStream();
        std::cout << "File stream state: "
                  << " good=" << file.good()
                  << " eof=" << file.eof()
                  << " fail=" << file.fail()
                  << " bad=" << file.bad() << std::endl;

        std::cout << "File position: " << file.tellg() << std::endl;
        // 创建磁盘管理器
        std::cout << "Creating DiskManager..." << std::endl;
        minidb::storage::DiskManager diskManager(fileManager);
        std::cout << "DiskManager created successfully" << std::endl;

        std::cout << "Initial page count: " << diskManager.getPageCount() << std::endl;
        std::cout << "Initial free list head: " << diskManager.getFreeListHead() << std::endl;

        // 测试页面分配
        std::cout << "\n--- Testing page allocation ---" << std::endl;
        std::vector<minidb::PageID> allocated_pages;
        for (int i = 0; i < 5; ++i) {
            std::cout << "Allocating page " << i << "..." << std::endl;
            auto page_id = diskManager.allocatePage();
            allocated_pages.push_back(page_id);
            std::cout << "Allocated page: " << page_id << std::endl;
        }

        std::cout << "Page count after allocation: " << diskManager.getPageCount() << std::endl;
        std::cout << "Free list head after allocation: " << diskManager.getFreeListHead() << std::endl;

        // 测试页面写入
        std::cout << "\n--- Testing page writing ---" << std::endl;
        char write_data[minidb::PAGE_SIZE];
        for (int i = 0; i < allocated_pages.size(); ++i) {
            std::memset(write_data, 0, minidb::PAGE_SIZE);
            std::string message = "Hello from page " + std::to_string(i);
            std::strncpy(write_data, message.c_str(), message.size());

            std::cout << "Writing to page " << allocated_pages[i] << ": " << message << std::endl;
            diskManager.writePage(allocated_pages[i], write_data);
            std::cout << "Write completed" << std::endl;
        }

        // 测试页面读取
        std::cout << "\n--- Testing page reading ---" << std::endl;
        char read_data[minidb::PAGE_SIZE];
        for (auto page_id : allocated_pages) {
            std::memset(read_data, 0, minidb::PAGE_SIZE);
            std::cout << "Reading from page " << page_id << "..." << std::endl;
            diskManager.readPage(page_id, read_data);
            std::cout << "Read from page " << page_id << ": " << read_data << std::endl;
        }

        // 测试页面释放
        std::cout << "\n--- Testing page deallocation ---" << std::endl;
        for (int i = 1; i < allocated_pages.size(); i += 2) {
            std::cout << "Deallocating page: " << allocated_pages[i] << std::endl;
            diskManager.deallocatePage(allocated_pages[i]);
            std::cout << "Deallocated page: " << allocated_pages[i] << std::endl;
            std::cout << "Current free list head: " << diskManager.getFreeListHead() << std::endl;
        }

        // 测试重新分配释放的页面
        std::cout << "\n--- Testing reallocation ---" << std::endl;
        for (int i = 0; i < 2; ++i) {
            std::cout << "Reallocating page..." << std::endl;
            auto new_page_id = diskManager.allocatePage();
            std::memset(write_data, 0, minidb::PAGE_SIZE);
            std::string message = "Reallocated page " + std::to_string(new_page_id);
            std::strncpy(write_data, message.c_str(), message.size());

            diskManager.writePage(new_page_id, write_data);
            diskManager.readPage(new_page_id, read_data);
            std::cout << "Reallocated and tested page: " << new_page_id << " - " << read_data << std::endl;
        }

        std::cout << "Final page count: " << diskManager.getPageCount() << std::endl;
        std::cout << "Final free list head: " << diskManager.getFreeListHead() << std::endl;
        std::cout << "DiskManager test completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "DiskManager test failed: " << e.what() << std::endl;
    }
}

// 简单的交互式测试菜单
void diskManagerTestMenu() {
    std::cout << "\n=== Disk Manager Test Menu ===" << std::endl;
    std::cout << "1. Run comprehensive test" << std::endl;
    std::cout << "2. Test page allocation" << std::endl;
    std::cout << "3. Test page read/write" << std::endl;
    std::cout << "4. Test page deallocation" << std::endl;
    std::cout << "5. Cleanup database" << std::endl;
    std::cout << "6. Back to main menu" << std::endl;
    std::cout << "Choose an option: ";

    std::string choice;
    std::getline(std::cin, choice);

    try {
        if (choice == "5") {
            cleanupDatabase();
            return;
        }
        if (choice == "6") {
            return;
        }

        auto fileManager = createFileManager();
        minidb::storage::DiskManager diskManager(fileManager);

        switch (std::stoi(choice)) {
            case 1:
                testDiskManager();
                break;
            case 2:
                testPageAllocation(diskManager);
                break;
            case 3:
                testPageReadWrite(diskManager);
                break;
            case 4:
                testPageDeallocation(diskManager);
                break;
            default:
                std::cout << "Invalid choice!" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// 主函数
int main() {
    std::cout << "=== DiskManager Test Program ===" << std::endl;

    while (true) {
        std::cout << "\nMain Menu:" << std::endl;
        std::cout << "1. Run DiskManager tests" << std::endl;
        std::cout << "2. Cleanup database" << std::endl;
        std::cout << "3. Exit" << std::endl;
        std::cout << "Choose an option: ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1") {
            diskManagerTestMenu();
        } else if (choice == "2") {
            cleanupDatabase();
        } else if (choice == "3") {
            std::cout << "Goodbye!" << std::endl;
            break;
        } else {
            std::cout << "Invalid choice!" << std::endl;
        }
    }

    return 0;
}