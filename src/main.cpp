#include "common/Value.h"
#include "common/Tuple.h"
#include "storage/FileManager.h"
#include "storage/DiskManager.h"
#include "storage/BufferManager.h"
#include "storage/Page.h"
#include <iostream>

// 测试基础类型
void testBasicTypes() {
    std::cout << "=== Testing Basic Types ===" << std::endl;

    minidb::Value int_val(42);
    minidb::Value str_val("Hello, MiniDB!");
    minidb::Value bool_val(true);

    std::cout << "Int value: " << int_val.toString() << std::endl;
    std::cout << "String value: " << str_val.toString() << std::endl;
    std::cout << "Bool value: " << bool_val.toString() << std::endl;

    // 测试 Tuple
    std::vector<minidb::Value> values = {int_val, str_val, bool_val};
    minidb::Tuple tuple(values);

    std::cout << "Tuple: " << tuple.toString() << std::endl;
    std::cout << "Tuple size: " << tuple.getColumnCount() << std::endl;
}

// 测试存储系统
void testStorageSystem() {
    std::cout << "\n=== Testing Storage System ===" << std::endl;

    try {
        // 创建文件管理器
        minidb::storage::FileManager file_manager;
        std::cout << "FileManager created." << std::endl;

        // 创建数据库
        file_manager.createDatabase("testdb");
        std::cout << "Database created." << std::endl;

        // 创建磁盘管理器
        minidb::storage::DiskManager disk_manager(&file_manager);
        std::cout << "DiskManager created. Page count: " << disk_manager.getPageCount() << std::endl;

        // 创建缓冲区管理器
        minidb::storage::BufferManager buffer_manager(&disk_manager);
        std::cout << "BufferManager created." << std::endl;

        // 分配新页面
        minidb::PageID page_id = disk_manager.allocatePage();
        std::cout << "Allocated page: " << page_id << std::endl;

        // 获取页面
        minidb::storage::Page* page = buffer_manager.fetchPage(page_id);
        std::cout << "Fetched page: " << page->toString() << std::endl;

        // 插入一些测试数据
        std::string test_data = "Hello, MiniDB!";
        minidb::RID rid;
        if (page->insertRecord(test_data.c_str(), test_data.size(), &rid)) {
            std::cout << "Inserted record with RID: " << rid.toString() << std::endl;
        } else {
            std::cout << "Failed to insert record." << std::endl;
        }

        // 读取记录
        char buffer[100];
        uint16_t size;
        if (page->getRecord(rid, buffer, &size)) {
            buffer[size] = '\0';
            std::cout << "Read record: " << buffer << std::endl;
        } else {
            std::cout << "Failed to read record." << std::endl;
        }

        // 标记页面为脏页
        buffer_manager.unpinPage(page_id, true);
        std::cout << "Page marked as dirty." << std::endl;

        // 刷新所有页面到磁盘
        buffer_manager.flushAllPages();
        std::cout << "All pages flushed to disk." << std::endl;

        // 关闭数据库
        file_manager.closeDatabase();
        std::cout << "Database closed." << std::endl;

    } catch (const minidb::DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Standard error: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "Unknown error occurred!" << std::endl;
        throw;
    }
}

int main() {
    try {
        std::cout << "Starting MiniDB test..." << std::endl;

        testBasicTypes();
        testStorageSystem();

        std::cout << "\nAll tests completed successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught in main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught in main!" << std::endl;
        return 1;
    }
}