// #include "../include/storage/Pager.h"
// #include "../include/engine/bPlusTree/bplus_tree.h"
// #include "../include/common/Value.h"
// #include "../include/common/Types.h"
// #include "../include/storage/BufferManager.h"
// #include "../include/storage/DiskManager.h"
// #include "../include/storage/FileManager.h"
// #include <memory>
// #include <iostream>
// #include <filesystem>
// #include <vector>
//
// using namespace minidb;
//
// // 清理测试文件
// void cleanup_test_files(const std::string& base_name) {
//     std::vector<std::string> extensions = {".minidb", ".db", ""};
//     for (const auto& ext : extensions) {
//         std::string filename = base_name + ext;
//         if (std::filesystem::exists(filename)) {
//             std::filesystem::remove(filename);
//         }
//     }
// }
//
// // 创建测试 Pager
// std::shared_ptr<storage::Pager> create_test_pager(const std::string& db_filename) {
//     cleanup_test_files(db_filename);
//
//     auto file_manager = std::make_shared<storage::FileManager>();
//     file_manager->createDatabase(db_filename);
//     file_manager->openDatabase(db_filename);
//
//     auto disk_manager = std::make_shared<storage::DiskManager>(file_manager);
//     auto buffer_manager = std::make_shared<storage::BufferManager>(disk_manager, 1000);
//
//     return std::make_shared<storage::Pager>(disk_manager, buffer_manager);
// }
//
// void test_pager_operations() {
//     std::cout << "\n=== Testing Pager Operations ===" << std::endl;
//
//     try {
//         auto pager = create_test_pager("pager_test.db");
//         std::cout << "Pager created successfully" << std::endl;
//
//         // 测试分配页面
//         std::cout << "Allocating page..." << std::endl;
//         auto page_id = pager->allocatePage();
//         std::cout << "Page allocated: " << page_id << std::endl;
//
//         // 测试获取页面 - 使用 getPage() 而不是 fetchPage()
//         std::cout << "Getting page..." << std::endl;
//         storage::Page* page = pager->getPage(page_id);
//         std::cout << "Page obtained: " << page->getPageId() << std::endl;
//
//         // 测试写入数据
//         std::cout << "Writing data to page..." << std::endl;
//         char* data = page->getData();
//         data[0] = 'T';
//         data[1] = 'E';
//         data[2] = 'S';
//         data[3] = 'T';
//
//         // 标记页面为脏
//         page->setDirty(true);
//
//         // 释放页面
//         std::cout << "Releasing page..." << std::endl;
//         pager->releasePage(page_id, true);
//
//         std::cout << "Pager operations test completed successfully" << std::endl;
//
//     } catch (const std::exception& e) {
//         std::cerr << "Pager test failed: " << e.what() << std::endl;
//         throw;
//     }
// }
//
// void test_bplus_tree_creation() {
//     std::cout << "\n=== Testing B+Tree Creation ===" << std::endl;
//
//     try {
//         auto pager = create_test_pager("tree_creation_test.db");
//         std::cout << "Pager created successfully" << std::endl;
//
//         // 测试创建 B+树
//         std::cout << "Creating B+Tree with INTEGER type..." << std::endl;
//         engine::BPlusTree tree(pager, -1, TypeId::INTEGER);
//         std::cout << "B+Tree created successfully" << std::endl;
//
//         // 测试基本属性
//         std::cout << "Tree height: " << tree.get_height() << std::endl;
//         std::cout << "Node count: " << tree.get_node_count() << std::endl;
//
//         std::cout << "B+Tree creation test completed successfully" << std::endl;
//
//     } catch (const std::exception& e) {
//         std::cerr << "B+Tree creation test failed: " << e.what() << std::endl;
//         throw;
//     }
// }
//
// void test_bplus_tree_search_empty() {
//     std::cout << "\n=== Testing B+Tree Search on Empty Tree ===" << std::endl;
//
//     try {
//         auto pager = create_test_pager("search_empty_test.db");
//         engine::BPlusTree tree(pager, -1, TypeId::INTEGER);
//
//         std::cout << "Searching for value 42 in empty tree..." << std::endl;
//         RID result = tree.search(Value(42));
//         std::cout << "Search result isValid: " << result.isValid() << std::endl;
//
//         if (!result.isValid()) {
//             std::cout << "? Correctly returned invalid RID for empty tree" << std::endl;
//         } else {
//             std::cout << "? Unexpected valid RID from empty tree" << std::endl;
//         }
//
//         std::cout << "Empty tree search test completed successfully" << std::endl;
//
//     } catch (const std::exception& e) {
//         std::cerr << "Empty tree search test failed: " << e.what() << std::endl;
//         throw;
//     }
// }
// void test_bplus_tree_single_insert() {
//     std::cout << "\n=== Testing B+Tree Single Insert ===" << std::endl;
//
//     try {
//         auto pager = create_test_pager("single_insert_test.db");
//         engine::BPlusTree tree(pager, -1, TypeId::INTEGER);
//
//         std::cout << "Initial tree state:" << std::endl;
//         std::cout << "Height: " << tree.get_height() << std::endl;
//         std::cout << "Node count: " << tree.get_node_count() << std::endl;
//         std::cout << "Root page ID: " << tree.get_root_page_id() << std::endl; // 添加这个
//
//         // 插入单个值
//         std::cout << "Inserting value 42 with RID(1, 10)..." << std::endl;
//         RID rid1(1, 10);
//
//         // 在插入前检查pager状态
//         std::cout << "Pager page count before insert: " << pager->getPageCount() << std::endl;
//
//         bool insert_result = tree.insert(Value(42), rid1);
//         std::cout << "Insert result: " << insert_result << std::endl;
//
//         // 在插入后检查状态
//         std::cout << "Pager page count after insert: " << pager->getPageCount() << std::endl;
//         std::cout << "Root page ID after insert: " << tree.get_root_page_id() << std::endl;
//
//         std::cout << "Tree state after insertion:" << std::endl;
//         std::cout << "Height: " << tree.get_height() << std::endl;
//         std::cout << "Node count: " << tree.get_node_count() << std::endl;
//
//         // 搜索插入的值
//         std::cout << "Searching for inserted value 42..." << std::endl;
//         RID search_result = tree.search(Value(42));
//         std::cout << "Search result isValid: " << search_result.isValid() << std::endl;
//
//         if (search_result.isValid()) {
//             std::cout << "Page ID: " << search_result.page_id << std::endl;
//             std::cout << "Slot Num: " << search_result.slot_num << std::endl;
//
//             if (search_result.page_id == 1 && search_result.slot_num == 10) {
//                 std::cout << "? Correct RID returned" << std::endl;
//             } else {
//                 std::cout << "? Incorrect RID returned" << std::endl;
//             }
//         }
//
//         std::cout << "Single insert test completed successfully" << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "Single insert test failed: " << e.what() << std::endl;
//         throw;
//     }
// }
//
//
// void test_bplus_tree_multiple_inserts() {
//     std::cout << "\n=== Testing B+Tree Multiple Inserts ===" << std::endl;
//
//     try {
//         auto pager = create_test_pager("multiple_inserts_test.db");
//         engine::BPlusTree tree(pager, -1, TypeId::INTEGER);
//
//         // 插入多个值
//         std::vector<int> values = {42, 25, 60, 10, 35, 50, 75};
//
//         for (size_t i = 0; i < values.size(); ++i) {
//             std::cout << "Inserting value " << values[i] << " with RID(" << i << ", " << i * 10 << ")..." << std::endl;
//             RID rid(i, i * 10);
//             bool result = tree.insert(Value(values[i]), rid);
//             std::cout << "Insert result: " << result << std::endl;
//         }
//
//         std::cout << "Final tree state:" << std::endl;
//         std::cout << "Height: " << tree.get_height() << std::endl;
//         std::cout << "Node count: " << tree.get_node_count() << std::endl;
//
//         // 验证所有插入的值都能找到
//         for (size_t i = 0; i < values.size(); ++i) {
//             RID result = tree.search(Value(values[i]));
//             if (result.isValid()) {
//                 std::cout << "? Found value " << values[i] << std::endl;
//             } else {
//                 std::cout << "? Missing value " << values[i] << std::endl;
//             }
//         }
//
//         // 测试不存在的值
//         RID not_found = tree.search(Value(999));
//         if (!not_found.isValid()) {
//             std::cout << "? Correctly did not find non-existent value 999" << std::endl;
//         } else {
//             std::cout << "? Unexpectedly found non-existent value 999" << std::endl;
//         }
//
//         std::cout << "Multiple inserts test completed successfully" << std::endl;
//
//     } catch (const std::exception& e) {
//         std::cerr << "Multiple inserts test failed: " << e.what() << std::endl;
//         throw;
//     }
// }
//
// void run_comprehensive_test() {
//     std::cout << "=== Starting Comprehensive B+Tree Test ===" << std::endl;
//
//     try {
//         // 运行各个测试模块
//         test_pager_operations();
//         test_bplus_tree_creation();
//         test_bplus_tree_search_empty();
//         test_bplus_tree_single_insert();
//         test_bplus_tree_multiple_inserts();
//
//         std::cout << "\n=== All tests completed successfully! ===" << std::endl;
//
//     } catch (const std::exception& e) {
//         std::cerr << "\n!!! TEST SUITE FAILED: " << e.what() << std::endl;
//         throw;
//     }
// }
//
// int main() {
//     std::cout << "Starting Comprehensive B+Tree Debug Test..." << std::endl;
//
//     try {
//         run_comprehensive_test();
//         std::cout << "\n? All tests passed! The B+Tree implementation appears to be working correctly." << std::endl;
//         return 0;
//
//     } catch (const std::exception& e) {
//         std::cerr << "\n? CRITICAL ERROR: " << e.what() << std::endl;
//         std::cerr << "Test terminated due to exception." << std::endl;
//         return 1;
//
//     } catch (...) {
//         std::cerr << "\n? UNKNOWN CRITICAL ERROR" << std::endl;
//         std::cerr << "Test terminated due to unknown exception." << std::endl;
//         return 1;
//     }
// }


#include <iostream>
#include <memory>
#include "../include/engine/ExecutionEngine.h"
#include "../include/engine/catalog/catalog_manager.h"
#include "../include/storage/BufferManager.h"
#include "../include/storage/DiskManager.h"
#include "../include/storage/FileManager.h" // 需要包含 FileManager 头文件
#include "json.hpp"  // 包含 JSON 库
#include "common/QueryResult.h"

using namespace minidb;

int main() {
    // 创建 FileManager 实例
    auto fileManager = std::make_shared<storage::FileManager>();
    std::string db_name = "my_database"; // 数据库名
    fileManager->createDatabase(db_name); // 创建数据库

    // 创建 DiskManager 实例
    auto diskManager = std::make_shared<storage::DiskManager>(fileManager);

    // 创建 CatalogManager 实例
    auto catalogManager = std::make_shared<CatalogManager>();

    // 创建 BufferManager 实例
    auto bufferManager = std::make_shared<storage::BufferManager>(diskManager);

    // 创建 ExecutionEngine 实例
    ExecutionEngine engine(catalogManager, bufferManager);

    // 1. 测试创建表
    nlohmann::json createTablePlan = {
        {"type", "CreateTable"},
        {"tableName", "Students"},
        {"columns", {
            {{"name", "name"}, {"type", "VARCHAR"}},
            {{"name", "age"}, {"type", "INTEGER"}}
        }}
    };

    // 执行创建表的操作
    engine.executePlan(createTablePlan);
    std::cout << "Table 'Students' created." << std::endl;

    // 2. 测试插入数据
    nlohmann::json insertPlan1 = {
        {"type", "Insert"},
        {"tableName", "Students"},
        {"values", {"Alice", "20"}}
    };

    engine.executePlan(insertPlan1);
    std::cout << "Inserted record into 'Students': Alice, 20." << std::endl;

    nlohmann::json insertPlan2 = {
        {"type", "Insert"},
        {"tableName", "Students"},
        {"values", {"Bob", "22"}}
    };
    engine.executePlan(insertPlan2);
    std::cout << "Inserted record into 'Students': Bob, 22." << std::endl;

    // 3. 测试选择数据
    nlohmann::json selectPlan = {
        {"type", "Select"},
        {"tableName", "Students"},
        {"columns", {"*"}}
    };

    QueryResult selectResult = engine.executePlan(selectPlan);

    // 打印选择结果
    std::cout << "Selected records:" << std::endl;
    for (size_t i = 0; i < selectResult.rowCount(); ++i) {
        const auto& row = selectResult.getRow(i);
        for (const auto& column : row) {
            std::cout << column << " ";
        }
        std::cout << std::endl;
    }

    // 4. 测试删除数据
    nlohmann::json deletePlan = {
        {"type", "Delete"},
        {"tableName", "Students"},
        {"condition", {
            {"column", "name"},
            {"op", "EQUALS"},
            {"value", "Alice"}
        }}
    };
    engine.executePlan(deletePlan);
    std::cout << "Deleted record from 'Students': Alice." << std::endl;

    // 验证删除后再进行选择
    selectResult = engine.executePlan(selectPlan);
    std::cout << "Selected records after deletion:" << std::endl;
    for (size_t i = 0; i < selectResult.rowCount(); ++i) {
        const auto& row = selectResult.getRow(i);
        for (const auto& column : row) {
            std::cout << column << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
