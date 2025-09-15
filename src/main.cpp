#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#include <cstring>

#include "../include/storage/FileManager.h"
#include "../include/storage/DiskManager.h"
#include "../include/storage/BufferManager.h"
#include "../include/storage/Pager.h"
#include "../include/storage/Page.h"
#include "../include/common/Constants.h"
#include "../include/common/Types.h"
#include "../include/common/Exception.h"


// // 简化命名空间
// using namespace minidb;
// using namespace minidb::storage;
// namespace fs = std::filesystem;
//
// // 打印测试结果（英文输出，避免乱码）
// void printTestResult(const std::string& test_name, bool success) {
//     if (success) {
//         std::cout << "\033[32m[PASS]\033[0m " << test_name << std::endl;
//     } else {
//         std::cout << "\033[31m[FAIL]\033[0m " << test_name << std::endl;
//     }
// }
//
// // 辅助函数：清理测试文件（适配 Windows 路径）
// void cleanupTestFiles(const std::string& db_name) {
//     std::string db_path = db_name + ".minidb";
//     // 统一路径分隔符为 Windows 格式
//     std::replace(db_path.begin(), db_path.end(), '/', '\\');
//     if (fs::exists(db_path)) {
//         fs::remove(db_path);
//         // 清理空目录（如果是测试用的临时目录）
//         std::string parent_dir = fs::path(db_path).parent_path().string();
//         if (fs::exists(parent_dir) && fs::is_directory(parent_dir) && fs::directory_iterator(parent_dir) == fs::directory_iterator()) {
//             fs::remove(parent_dir);
//         }
//     }
// }
//
// int main() {
//     std::cout << "===================== FileManager Test =====================" << std::endl;
//     const std::string TEST_DB = "test_file_manager";
//     // 修复：Windows 下明确不可写的绝对路径（C盘根目录下的未知目录）
//     const std::string INVALID_DB = "C:\\InvalidDir_1234\\test_db";
//     bool all_pass = true;
//
//     FileManager file_manager;
//
//     // 1. Test initial state (not open)
//     try {
//         bool is_open = file_manager.isOpen();
//         printTestResult("Initial state: Database not open", !is_open);
//     } catch (...) {
//         printTestResult("Initial state: Database not open", false);
//         all_pass = false;
//     }
//
//     // 2. Test create database (normal case)
//     try {
//         cleanupTestFiles(TEST_DB);
//         file_manager.createDatabase(TEST_DB);
//         bool is_open = file_manager.isOpen();
//         bool file_exists = fs::exists(file_manager.getDatabasePath());
//         printTestResult("Create database (normal case)", is_open && file_exists);
//     } catch (const DirectoryCreationException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Create db failed (dir issue): " << e.what() << std::endl;
//         printTestResult("Create database (normal case)", false);
//         all_pass = false;
//     } catch (const FileException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Create db failed (file issue): " << e.what() << std::endl;
//         printTestResult("Create database (normal case)", false);
//         all_pass = false;
//     }
//
//     // 3. Test get database path
//     try {
//         std::string expected_path = TEST_DB + ".minidb";
//         std::replace(expected_path.begin(), expected_path.end(), '/', '\\');
//         std::string actual_path = file_manager.getDatabasePath();
//         printTestResult("Get database path (" + actual_path + ")", actual_path == expected_path);
//     } catch (const DatabaseNotOpenException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Get path failed: " << e.what() << std::endl;
//         printTestResult("Get database path", false);
//         all_pass = false;
//     }
//
//     // 4. Test re-create database (auto close existing)
//     try {
//         file_manager.createDatabase(TEST_DB);
//         bool is_open = file_manager.isOpen();
//         printTestResult("Re-create database (auto close existing)", is_open);
//     } catch (const FileException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Re-create db failed: " << e.what() << std::endl;
//         printTestResult("Re-create database", false);
//         all_pass = false;
//     }
//
//     // 5. Test close database
//     try {
//         file_manager.closeDatabase();
//         printTestResult("Close database", !file_manager.isOpen());
//     } catch (const FileException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Close db failed: " << e.what() << std::endl;
//         printTestResult("Close database", false);
//         all_pass = false;
//     }
//
//     // 6. Test open existing database
//     try {
//         file_manager.openDatabase(TEST_DB);
//         printTestResult("Open existing database", file_manager.isOpen());
//     } catch (const FileNotFoundException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Open failed (file not found): " << e.what() << std::endl;
//         printTestResult("Open existing database", false);
//         all_pass = false;
//     } catch (const FileException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Open failed: " << e.what() << std::endl;
//         printTestResult("Open existing database", false);
//         all_pass = false;
//     }
//
//     // 7. Test get file stream (open state)
//     try {
//         std::fstream& stream = file_manager.getFileStream();
//         printTestResult("Get file stream (open state)", stream.is_open());
//     } catch (const DatabaseNotOpenException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Get stream failed: " << e.what() << std::endl;
//         printTestResult("Get file stream (open state)", false);
//         all_pass = false;
//     }
//
//     // 8. Test get stream after close (exception)
//     try {
//         file_manager.closeDatabase();
//         bool exception_caught = false;
//         try {
//             file_manager.getFileStream();
//         } catch (const DatabaseNotOpenException& e) {
//             exception_caught = true;
//         }
//         printTestResult("Get stream after close (exception test)", exception_caught);
//     } catch (const FileException& e) {
//         std::cout << "\033[31m[ERROR]\033[0m Close failed: " << e.what() << std::endl;
//         printTestResult("Get stream after close", false);
//         all_pass = false;
//     }
//
//     // 9. Test open non-existent database (exception)
//     try {
//         bool exception_caught = false;
//         try {
//             file_manager.openDatabase("non_existent_db_12345");
//         } catch (const FileNotFoundException& e) {
//             exception_caught = true;
//         }
//         printTestResult("Open non-existent database (exception test)", exception_caught);
//     } catch (...) {
//         printTestResult("Open non-existent database (exception test)", false);
//         all_pass = false;
//     }
//
//     // 10. Test create db in invalid directory (exception, Windows-specific)
//     try {
//         bool exception_caught = false;
//         try {
//             file_manager.createDatabase(INVALID_DB);
//             // 若未抛出异常，手动检查是否真的创建成功（防止系统兼容）
//             if (fs::exists(file_manager.getDatabasePath())) {
//                 cleanupTestFiles(INVALID_DB); // 清理意外创建的文件
//                 exception_caught = false;
//             }
//         } catch (const DirectoryCreationException& e) {
//             exception_caught = true;
//         } catch (const FileException& e) {
//             // 部分 Windows 环境可能抛出 FileException（而非 DirectoryCreationException），也视为通过
//             exception_caught = true;
//         }
//         printTestResult("Create db in invalid directory (exception test)", exception_caught);
//     } catch (...) {
//         printTestResult("Create db in invalid directory (exception test)", false);
//         all_pass = false;
//     }
//
//     // Cleanup
//     cleanupTestFiles(TEST_DB);
//     cleanupTestFiles(INVALID_DB);
//
//     // Summary
//     std::cout << "\n===================== Test Summary =====================" << std::endl;
//     if (all_pass) {
//         std::cout << "\033[32mAll FileManager tests passed!\033[0m" << std::endl;
//     } else {
//         std::cout << "\033[31mSome FileManager tests failed! Check logs.\033[0m" << std::endl;
//     }
//
//     return all_pass ? 0 : 1;
// }

// 简化命名空间
using namespace minidb;
using namespace minidb::storage;
namespace fs = std::filesystem;

// 打印测试结果
void printTestResult(const std::string& test_name, bool success) {
    if (success) {
        std::cout << "\033[32m[PASS]\033[0m " << test_name << std::endl;
    } else {
        std::cout << "\033[31m[FAIL]\033[0m " << test_name << std::endl;
    }
}

// 清理测试文件
void cleanupTestFiles(const std::string& db_name) {
    std::string db_path = db_name + ".minidb";
    std::replace(db_path.begin(), db_path.end(), '/', '\\');
    if (fs::exists(db_path)) {
        fs::remove(db_path);
    }
}

int main() {
    std::cout << "===================== DiskManager Test =====================" << std::endl;
    const std::string TEST_DB = "test_disk_manager";
    bool all_pass = true;

    // 初始化依赖组件
    auto file_manager = std::make_shared<FileManager>();
    std::shared_ptr<DiskManager> disk_manager;

    // 1. 测试初始化（依赖FileManager）
    try {
        cleanupTestFiles(TEST_DB);
        file_manager->createDatabase(TEST_DB);
        disk_manager = std::make_shared<DiskManager>(file_manager);

        // 验证初始页计数（0号为元数据页，初始总页数应为1）
        bool init_success = (disk_manager->getPageCount() == 1) &&
                          (disk_manager->getFreeListHead() == INVALID_PAGE_ID);
        printTestResult("Initialize DiskManager (PageCount=1, FreeListHead=-1)", init_success);
    } catch (const DatabaseException& e) {
        std::cout << "\033[31m[ERROR]\033[0m Initialization failed: " << e.what() << std::endl;
        printTestResult("Initialize DiskManager", false);
        return 1; // 初始化失败，后续测试无法进行
    }

    // 2. 测试分配新页面
    try {
        PageID new_page_id = disk_manager->allocatePage();
        // 新页面ID应为1（0号是元数据页），且页计数应增加到2
        bool alloc_success = (new_page_id == 1) && (disk_manager->getPageCount() == 2);
        printTestResult("Allocate new page (ID=1, PageCount=2)", alloc_success);
    } catch (const DiskException& e) {
        std::cout << "\033[31m[ERROR]\033[0m Page allocation failed: " << e.what() << std::endl;
        printTestResult("Allocate new page", false);
        all_pass = false;
    }

    // 3. 测试写入和读取页面
    try {
        // 准备测试数据（填充一个完整页面）
        char write_buffer[PAGE_SIZE];
        memset(write_buffer, 0xAA, PAGE_SIZE); // 填充测试数据0xAA
        PageID test_page_id = 1; // 使用步骤2分配的页面

        // 写入页面
        disk_manager->writePage(test_page_id, write_buffer);

        // 读取页面并验证
        char read_buffer[PAGE_SIZE];
        disk_manager->readPage(test_page_id, read_buffer);
        bool data_match = (memcmp(write_buffer, read_buffer, PAGE_SIZE) == 0);

        printTestResult("Write and read page (data match)", data_match);
    } catch (const IOException& e) {
        std::cout << "\033[31m[ERROR]\033[0m Read/write failed: " << e.what() << std::endl;
        printTestResult("Write and read page", false);
        all_pass = false;
    } catch (const PageOutOfRangeException& e) {
        std::cout << "\033[31m[ERROR]\033[0m Page ID out of range: " << e.what() << std::endl;
        printTestResult("Write and read page", false);
        all_pass = false;
    }

    // 4. 测试回收页面（加入空闲链表）
    try {
        PageID page_to_free = 1;
        disk_manager->deallocatePage(page_to_free);

        // 验证：空闲链表头应指向被回收的页面
        bool free_success = (disk_manager->getFreeListHead() == page_to_free);
        printTestResult("Deallocate page (add to free list)", free_success);
    } catch (const DiskException& e) {
        std::cout << "\033[31m[ERROR]\033[0m Page deallocation failed: " << e.what() << std::endl;
        printTestResult("Deallocate page", false);
        all_pass = false;
    }

    // 5. 测试从空闲链表分配页面
    try {
        // 此时空闲链表有一个页面（ID=1），再次分配应复用该页面
        PageID reused_page_id = disk_manager->allocatePage();
        bool reuse_success = (reused_page_id == 1) && (disk_manager->getFreeListHead() == INVALID_PAGE_ID);
        printTestResult("Allocate from free list (reuse page 1)", reuse_success);
    } catch (const DiskException& e) {
        std::cout << "\033[31m[ERROR]\033[0m Reuse free page failed: " << e.what() << std::endl;
        printTestResult("Allocate from free list", false);
        all_pass = false;
    }

    // 6. 测试读取无效页面ID（异常场景）
    try {
        char buffer[PAGE_SIZE];
        bool exception_caught = false;
        try {
            disk_manager->readPage(999, buffer); // 999是无效ID（当前页计数为2）
        } catch (const PageOutOfRangeException& e) {
            exception_caught = true;
        }
        printTestResult("Read invalid page ID (999) - exception test", exception_caught);
    } catch (const DatabaseException& e) {
        std::cout << "\033[31m[ERROR]\033[0m Unexpected exception: " << e.what() << std::endl;
        printTestResult("Read invalid page ID", false);
        all_pass = false;
    }

    // 7. 测试刷盘（确保元数据持久化）
    try {
        // 先分配新页面，修改元数据
        disk_manager->allocatePage(); // 此时页计数应为3
        disk_manager->flush(); // 刷盘

        // 重新打开数据库验证元数据是否持久化
        file_manager->closeDatabase();
        file_manager->openDatabase(TEST_DB);
        auto new_disk_manager = std::make_shared<DiskManager>(file_manager);
        bool flush_success = (new_disk_manager->getPageCount() == 3);

        printTestResult("Flush metadata (page count persisted)", flush_success);
    } catch (const DatabaseException& e) {
        std::cout << "\033[31m[ERROR]\033[0m Flush failed: " << e.what() << std::endl;
        printTestResult("Flush metadata", false);
        all_pass = false;
    }

    // 清理测试文件
    cleanupTestFiles(TEST_DB);

    // 测试总结
    std::cout << "\n===================== Test Summary =====================" << std::endl;
    if (all_pass) {
        std::cout << "\033[32mAll DiskManager tests passed!\033[0m" << std::endl;
    } else {
        std::cout << "\033[31mSome DiskManager tests failed! Check logs.\033[0m" << std::endl;
    }

    return all_pass ? 0 : 1;
}
