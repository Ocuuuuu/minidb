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
// // ??????????
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
// // ???????? Pager
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
//         // ??????????
//         std::cout << "Allocating page..." << std::endl;
//         auto page_id = pager->allocatePage();
//         std::cout << "Page allocated: " << page_id << std::endl;
//
//         // ????????? - ??? getPage() ?????? fetchPage()
//         std::cout << "Getting page..." << std::endl;
//         storage::Page* page = pager->getPage(page_id);
//         std::cout << "Page obtained: " << page->getPageId() << std::endl;
//
//         // ????????????
//         std::cout << "Writing data to page..." << std::endl;
//         char* data = page->getData();
//         data[0] = 'T';
//         data[1] = 'E';
//         data[2] = 'S';
//         data[3] = 'T';
//
//         // ?????????
//         page->setDirty(true);
//
//         // ??????
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
//         // ??????? B+??
//         std::cout << "Creating B+Tree with INTEGER type..." << std::endl;
//         engine::BPlusTree tree(pager, -1, TypeId::INTEGER);
//         std::cout << "B+Tree created successfully" << std::endl;
//
//         // ???????????
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
//         std::cout << "Root page ID: " << tree.get_root_page_id() << std::endl; // ??????
//
//         // ???????
//         std::cout << "Inserting value 42 with RID(1, 10)..." << std::endl;
//         RID rid1(1, 10);
//
//         // ?????????pager??
//         std::cout << "Pager page count before insert: " << pager->getPageCount() << std::endl;
//
//         bool insert_result = tree.insert(Value(42), rid1);
//         std::cout << "Insert result: " << insert_result << std::endl;
//
//         // ??????????
//         std::cout << "Pager page count after insert: " << pager->getPageCount() << std::endl;
//         std::cout << "Root page ID after insert: " << tree.get_root_page_id() << std::endl;
//
//         std::cout << "Tree state after insertion:" << std::endl;
//         std::cout << "Height: " << tree.get_height() << std::endl;
//         std::cout << "Node count: " << tree.get_node_count() << std::endl;
//
//         // ??????????
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
//         // ???????
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
//         // ????????????????????
//         for (size_t i = 0; i < values.size(); ++i) {
//             RID result = tree.search(Value(values[i]));
//             if (result.isValid()) {
//                 std::cout << "? Found value " << values[i] << std::endl;
//             } else {
//                 std::cout << "? Missing value " << values[i] << std::endl;
//             }
//         }
//
//         // ???????????
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
//         // ???????????????
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
//
// #include <iostream>
// #include <memory>
// #include "../include/engine/ExecutionEngine.h"
// #include "../include/engine/catalog/catalog_manager.h"
// #include "../include/storage/BufferManager.h"
// #include "../include/storage/DiskManager.h"
// #include "../include/storage/FileManager.h"
// #include "json.hpp"
// #include "common/QueryResult.h"
//
// using namespace minidb;
//
// int main() {
//     try {
//         std::cout << "Starting MiniDB test..." << std::endl;
//
//         // Initialize components
//         auto fileManager = std::make_shared<storage::FileManager>();
//         std::string db_name = "my_database";
//
//         // Clean up any existing database
//         if (fileManager->databaseExists(db_name)) {
//             std::cout << "Removing existing database..." << std::endl;
//             fileManager->deleteDatabase(db_name);
//         }
//
//         fileManager->createDatabase(db_name);
//         std::cout << "Database created: " << db_name << std::endl;
//
//         auto diskManager = std::make_shared<storage::DiskManager>(fileManager);
//         auto catalogManager = std::make_shared<CatalogManager>();
//         auto bufferManager = std::make_shared<storage::BufferManager>(diskManager, 100);
//
//         ExecutionEngine engine(catalogManager, bufferManager);
//
//         // 1. Create table
//         std::cout << "Creating table..." << std::endl;
//         nlohmann::json createTablePlan = {
//             {"type", "CreateTable"},
//             {"tableName", "Students"},
//             {"columns", {
//                 {{"name", "name"}, {"type", "VARCHAR"}},
//                 {{"name", "age"}, {"type", "INTEGER"}}
//             }}
//         };
//
//         engine.executePlan(createTablePlan);
//         std::cout << "Table 'Students' created successfully." << std::endl;
//
//         // Verify table exists
//         bool tableExists = catalogManager->table_exists("Students");
//         if (tableExists) {
//             std::cout << "Table verified in catalog: Students" << std::endl;
//             auto table = catalogManager->get_table("Students");
//             if (table) {
//                 std::cout << "Table pointer obtained successfully" << std::endl;
//                 std::cout << "Table name: " << table->get_table_name() << std::endl;
//             }
//         } else {
//             std::cerr << "ERROR: Table not found in catalog!" << std::endl;
//             return 1;
//         }
//
//         // 2. Insert records - 修改数据类型
//         std::cout << "Inserting records..." << std::endl;
//
//         nlohmann::json insertPlan1 = {
//             {"type", "Insert"},
//             {"tableName", "Students"},
//             {"values", {"Alice", "20"}}
//         };
//
//         try {
//             engine.executePlan(insertPlan1);
//             std::cout << "Inserted record: Alice, 20" << std::endl;
//         } catch (const std::exception& e) {
//             std::cerr << "Insert failed: " << e.what() << std::endl;
//
//             // 尝试获取更多调试信息
//             try {
//                 auto table = catalogManager->get_table("Students");
//                 if (table) {
//                     std::cout << "Table exists but insert failed" << std::endl;
//                 }
//             } catch (...) {
//                 std::cerr << "Could not get table for debugging" << std::endl;
//             }
//
//             return 1;
//         }
//
//         nlohmann::json insertPlan2 = {
//             {"type", "Insert"},
//             {"tableName", "Students"},
//             {"values", {"Bob", "22"}}
//         };
//
//         try {
//             engine.executePlan(insertPlan2);
//             std::cout << "Inserted record: Bob, 22" << std::endl;
//         } catch (const std::exception& e) {
//             std::cerr << "Insert failed: " << e.what() << std::endl;
//             return 1;
//         }
//
//         // 其余代码保持不变...
//         // 3. Select records
//         std::cout << "Selecting records..." << std::endl;
//         nlohmann::json selectPlan = {
//             {"type", "Select"},
//             {"tableName", "Students"},
//             {"columns", {"*"}}
//         };
//
//         try {
//             QueryResult selectResult = engine.executePlan(selectPlan);
//             std::cout << "Selected " << selectResult.rowCount() << " records:" << std::endl;
//
//             if (selectResult.rowCount() > 0) {
//                 // Print column headers
//                 const auto& firstRow = selectResult.getRow(0);
//                 std::cout << "Columns: ";
//                 for (size_t i = 0; i < firstRow.size(); ++i) {
//                     std::cout << "col" << i << " ";
//                 }
//                 std::cout << std::endl;
//
//                 // Print data
//                 for (size_t i = 0; i < selectResult.rowCount(); ++i) {
//                     const auto& row = selectResult.getRow(i);
//                     std::cout << "Row " << i << ": ";
//                     for (const auto& column : row) {
//                         std::cout << column << " ";
//                     }
//                     std::cout << std::endl;
//                 }
//             } else {
//                 std::cout << "No records found" << std::endl;
//             }
//         } catch (const std::exception& e) {
//             std::cerr << "Select failed: " << e.what() << std::endl;
//             return 1;
//         }
//
//         std::cout << "All operations completed successfully!" << std::endl;
//         return 0;
//
//     } catch (const std::exception& e) {
//         std::cerr << "Fatal error: " << e.what() << std::endl;
//         return 1;
//     }
// }







//====================================执行引擎的
// #include <iostream>
// #include <fstream>
// #include <memory>
// #include "storage/FileManager.h"
// #include "storage/DiskManager.h"
// #include "storage/BufferManager.h"
// #include "engine/catalog/catalog_manager.h"
// #include "engine/ExecutionEngine.h"
// #include "common/QueryResult.h"
// #include "json.hpp"
//
// using namespace minidb;
// using namespace minidb::storage;
// using json = nlohmann::json;
//
// // ====== 阶段 1：初始化数据库 ======
// void initPhase() {
//     std::cout << "=== INIT MODE ===" << std::endl;
//
//     // 如果存在旧数据库文件，删除
//     if (std::filesystem::exists("testdb")) {
//         std::cout << "Database file already exists. Deleting it..." << std::endl;
//         std::filesystem::remove("testdb");
//     }
//
//     // 创建一个新数据库
//     auto fileManager = std::make_shared<FileManager>();
//     fileManager->createDatabase("testdb");
//
//     auto diskManager = std::make_shared<DiskManager>(fileManager);
//     auto bufferManager = std::make_shared<BufferManager>(diskManager, 10);
//     auto catalog = std::make_shared<CatalogManager>();
//     ExecutionEngine exec(catalog, bufferManager);
//
//     // 创建表 users
//     json createPlan = {
//         {"type", "CreateTable"},
//         {"tableName", "users"},
//         {"columns", {
//             {{"name", "id"},   {"type", "INTEGER"}},
//             {{"name", "name"}, {"type", "VARCHAR"}},
//             {{"name", "age"},  {"type", "INTEGER"}}
//         }}
//     };
//     exec.executePlan(createPlan);
//     std::cout << "[OK] Table created.\n";
//
//     // 插入 Alice
//     json insert1 = {
//         {"type", "Insert"},
//         {"tableName", "users"},
//         {"values", {"1", "Alice", "30"}}
//     };
//     exec.executePlan(insert1);
//
//     // 插入 Bob
//     json insert2 = {
//         {"type", "Insert"},
//         {"tableName", "users"},
//         {"values", {"2", "Bob", "25"}}
//     };
//     exec.executePlan(insert2);
//     std::cout << "[OK] 2 rows inserted.\n";
//
//     // SELECT 检查
//     json selectPlan = {
//         {"type", "Select"},
//         {"tableName", "users"},
//         {"columns", {"id", "name", "age"}}
//     };
//     QueryResult result = exec.executePlan(selectPlan);
//     result.print();
//
//     // === 保存 meta 文件 ===
//     TableInfo *tinfo = catalog->get_table("users");
//     if (!tinfo) throw std::runtime_error("Failed to get table info after create_table");
//
//     json meta;
//     meta["tableName"] = "users";
//     meta["columns"] = json::array();
//     meta["columns"].push_back({{"name", "id"},   {"type", "INTEGER"}});
//     meta["columns"].push_back({{"name", "name"}, {"type", "VARCHAR"}});
//     meta["columns"].push_back({{"name", "age"},  {"type", "INTEGER"}});
//     meta["firstPageID"] = tinfo->getFirstPageID();
//
//     std::ofstream out("catalog.meta");
//     out << meta.dump(4);
//     out.close();
//
//     std::cout << "=== 模拟重启数据库 ===" << std::endl;
// }
//
// // ====== 阶段 2：重启后读取 ======
// void readPhase() {
//     std::cout << "=== READ MODE ===" << std::endl;
//
//     // 打开数据库
//     auto fileManager = std::make_shared<FileManager>();
//     fileManager->openDatabase("testdb");
//
//     auto diskManager = std::make_shared<DiskManager>(fileManager);
//     auto bufferManager = std::make_shared<BufferManager>(diskManager, 10);
//     auto catalog = std::make_shared<CatalogManager>();
//     ExecutionEngine exec(catalog, bufferManager);
//
//     // === 从 meta 文件恢复 Catalog ===
//     std::ifstream in("catalog.meta");
//     if (!in.is_open()) throw std::runtime_error("Failed to open catalog.meta");
//     json meta;
//     in >> meta;
//     in.close();
//
//     Schema schema;
//     for (auto &col : meta["columns"]) {
//         std::string name = col["name"];
//         std::string type = col["type"];
//
//         if (type == "INTEGER") {
//             schema.addColumn(MyColumn(name, TypeId::INTEGER, 4, schema.get_length()));
//         } else if (type == "VARCHAR") {
//             schema.addColumn(MyColumn(name, TypeId::VARCHAR, 255, schema.get_length()));
//         }
//     }
//     catalog->create_table(meta["tableName"], schema);
//     catalog->get_table(meta["tableName"])->setFirstPageID(meta["firstPageID"]);
//
//     // === SELECT After Restart ===
//     json selectPlan = {
//         {"type", "Select"},
//         {"tableName", "users"},
//         {"columns", {"id", "name", "age"}}
//     };
//     QueryResult result = exec.executePlan(selectPlan);
//     std::cout << "--- SELECT After Restart ---" << std::endl;
//     result.print();
//
//     // UPDATE Bob's age
//     json updatePlan = {
//         {"type", "Update"},
//         {"tableName", "users"},
//         {"updates", {
//             {{"column", "age"}, {"value", "26"}}
//         }}
//     };
//     exec.executePlan(updatePlan);
//     std::cout << "[OK] Updated Bob's age.\n";
//
//     // DELETE Alice
//     json deletePlan = {
//         {"type", "Delete"},
//         {"tableName", "users"},
//         {"condition", {
//             {"column", "name"},
//             {"value", "Alice"},
//             {"op", "EQUALS"}
//         }}
//     };
//     exec.executePlan(deletePlan);
//     std::cout << "[OK] Deleted Alice.\n";
//
//     // Final SELECT
//     result = exec.executePlan(selectPlan);
//     std::cout << "--- SELECT Final ---" << std::endl;
//     result.print();
// }
//
// // ====== 主入口 ======
// int main() {
//     try {
//         initPhase();
//         readPhase();
//     } catch (const std::exception &e) {
//         std::cerr << "[ERROR] " << e.what() << std::endl;
//         return 1;
//     }
//     return 0;
// }