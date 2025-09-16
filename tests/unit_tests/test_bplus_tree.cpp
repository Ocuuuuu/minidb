// #include <../tests/catch2/catch_amalgamated.hpp>
// #include "storage/Pager.h"
// #include "engine/bPlusTree/bplus_tree.h"
// #include "common/Value.h"
// #include "common/Types.h"
// #include "common/Exception.h"
// #include "storage/BufferManager.h"
// #include "storage/DiskManager.h"
// #include <memory>
// #include <vector>
// #include <algorithm>
// #include <random>
// #include <iostream>
// #include <filesystem>
//
// using namespace minidb;
//
// // 测试配置
// constexpr const char* TEST_DB_FILE = "test_bplus_tree.db";
// constexpr int TEST_PAGE_SIZE = 4096;
// constexpr PageID TEST_INVALID_PAGE_ID = -1;
//
// // 测试辅助函数
// namespace test_utils {
//
//     // 创建测试用的Pager
//     std::shared_ptr<storage::Pager> create_test_pager() {
//         // 先清理旧测试文件
//         if (std::filesystem::exists(TEST_DB_FILE)) {
//             std::filesystem::remove(TEST_DB_FILE);
//         }
//
//         // 创建FileManager、DiskManager和BufferManager
//         auto file_manager = std::make_shared<storage::FileManager>();
//         file_manager->createDatabase(TEST_DB_FILE); // 创建数据库文件
//         file_manager->openDatabase(TEST_DB_FILE);   // 打开数据库文件
//
//         auto disk_manager = std::make_shared<storage::DiskManager>(file_manager);
//         auto buffer_manager = std::make_shared<storage::BufferManager>(disk_manager, 1000);
//
//         return std::make_shared<storage::Pager>(disk_manager, buffer_manager);
//     }
//
//     // 生成随机整数
//     int generate_random_int(int min = 0, int max = 1000) {
//         static std::random_device rd;
//         static std::mt19937 gen(rd());
//         std::uniform_int_distribution<> distrib(min, max);
//         return distrib(gen);
//     }
//
//     // 生成测试数据
//     std::vector<std::pair<Value, RID>> generate_test_data(int count, int start = 0) {
//         std::vector<std::pair<Value, RID>> data;
//         data.reserve(count);
//
//         for (int i = start; i < start + count; ++i) {
//             data.emplace_back(Value(i), RID{i % 10 + 1, i % 100 + 1});
//         }
//         return data;
//     }
//
//     // 生成随机测试数据
//     std::vector<std::pair<Value, RID>> generate_random_test_data(int count) {
//         std::vector<std::pair<Value, RID>> data;
//         data.reserve(count);
//
//         for (int i = 0; i < count; ++i) {
//             int key = generate_random_int(0, count * 2);
//             data.emplace_back(Value(key), RID{key % 10 + 1, key % 100 + 1});
//         }
//         return data;
//     }
//
// } // namespace test_utils
//
// TEST_CASE("BPlusTree Basic Operations", "[bplustree][basic]") {
//     std::cout << "=== Starting Basic Operations Test ===" << std::endl;
//     auto pager = test_utils::create_test_pager();
//     engine::BPlusTree tree(pager, TEST_INVALID_PAGE_ID, TypeId::INTEGER);
//
//     SECTION("Empty tree operations") {
//         REQUIRE(tree.search(Value(42)).isValid() == false);
//         REQUIRE(tree.get_height() == 0);
//         REQUIRE(tree.get_node_count() == 0);
//     }
//
//     SECTION("Single insertion and search") {
//         RID rid1(1, 10);
//         REQUIRE(tree.insert(Value(42), rid1));
//
//         RID result = tree.search(Value(42));
//         REQUIRE(result.isValid());
//         REQUIRE(result.page_id == 1);
//         REQUIRE(result.slot_num == 10);
//
//         // 搜索不存在的键
//         REQUIRE(tree.search(Value(999)).isValid() == false);
//     }
//
//     SECTION("Multiple insertions and searches") {
//         std::vector<RID> rids = {
//             RID{1, 10},
//             RID{2, 20},
//             RID{3, 30},
//             RID{4, 40}
//         };
//
//         std::vector<Value> keys = {
//             Value(10),
//             Value(20),
//             Value(30),
//             Value(40)
//         };
//
//         // 插入所有键值对
//         for (size_t i = 0; i < keys.size(); ++i) {
//             REQUIRE(tree.insert(keys[i], rids[i]));
//         }
//
//         // 验证所有键都能找到
//         for (size_t i = 0; i < keys.size(); ++i) {
//             RID result = tree.search(keys[i]);
//             REQUIRE(result.isValid());
//             REQUIRE(result.page_id == rids[i].page_id);
//             REQUIRE(result.slot_num == rids[i].slot_num);
//         }
//
//         // 验证不存在的键返回无效RID
//         REQUIRE(tree.search(Value(99)).isValid() == false);
//     }
// }
//
// TEST_CASE("BPlusTree Leaf Node Splitting", "[bplustree][splitting]") {
//     std::cout << "=== Starting Basic Operations Test ===" << std::endl;
//     auto pager = test_utils::create_test_pager();
//     engine::BPlusTree tree(pager, TEST_INVALID_PAGE_ID, TypeId::INTEGER);
//
//     SECTION("Insert enough data to trigger splitting") {
//         const int test_count = 50;
//         auto test_data = test_utils::generate_test_data(test_count);
//
//         // 插入所有数据
//         for (const auto& [key, rid] : test_data) {
//             REQUIRE(tree.insert(key, rid));
//         }
//
//         // 验证所有数据都能正确检索
//         for (const auto& [key, rid] : test_data) {
//             RID result = tree.search(key);
//             REQUIRE(result.isValid());
//             REQUIRE(result.page_id == rid.page_id);
//             REQUIRE(result.slot_num == rid.slot_num);
//         }
//
//         // 树应该有高度（至少1层）和多个节点
//         REQUIRE(tree.get_height() >= 1);
//         REQUIRE(tree.get_node_count() >= 2);
//
//         std::cout << "Tree height after " << test_count << " inserts: " << tree.get_height() << std::endl;
//         std::cout << "Tree node count: " << tree.get_node_count() << std::endl;
//     }
// }
//
// TEST_CASE("BPlusTree Range Search", "[bplustree][range]") {
//     std::cout << "=== Starting Basic Operations Test ===" << std::endl;
//     auto pager = test_utils::create_test_pager();
//     engine::BPlusTree tree(pager, TEST_INVALID_PAGE_ID, TypeId::INTEGER);
//
//     SECTION("Basic range search") {
//         // 插入有序数据
//         for (int i = 0; i < 20; ++i) {
//             REQUIRE(tree.insert(Value(i), RID{i, i * 10}));
//         }
//
//         // 范围查询 [5, 15]
//         auto results = tree.range_search(Value(5), Value(15));
//         REQUIRE(results.size() == 11); // 5到15 inclusive
//
//         // 验证结果顺序和正确性
//         for (int i = 0; i < results.size(); ++i) {
//             REQUIRE(results[i].page_id == 5 + i);
//             REQUIRE(results[i].slot_num == (5 + i) * 10);
//         }
//     }
//
//     SECTION("Empty range search") {
//         for (int i = 0; i < 10; ++i) {
//             REQUIRE(tree.insert(Value(i), RID{i, i}));
//         }
//
//         // 查询不存在的范围
//         auto results = tree.range_search(Value(20), Value(30));
//         REQUIRE(results.empty());
//     }
// }
//
// TEST_CASE("BPlusTree Edge Cases", "[bplustree][edge]") {
//     std::cout << "=== Starting Basic Operations Test ===" << std::endl;
//     auto pager = test_utils::create_test_pager();
//     engine::BPlusTree tree(pager, TEST_INVALID_PAGE_ID, TypeId::INTEGER);
//
//     SECTION("Negative keys") {
//         RID rid(1, 100);
//         REQUIRE(tree.insert(Value(-5), rid));
//
//         RID result = tree.search(Value(-5));
//         REQUIRE(result.isValid());
//         REQUIRE(result.page_id == 1);
//         REQUIRE(result.slot_num == 100);
//     }
//
//     SECTION("Zero key") {
//         RID rid(1, 0);
//         REQUIRE(tree.insert(Value(0), rid));
//
//         RID result = tree.search(Value(0));
//         REQUIRE(result.isValid());
//         REQUIRE(result.page_id == 1);
//     }
//
//     SECTION("Large keys") {
//         RID rid(1, 999);
//         REQUIRE(tree.insert(Value(INT32_MAX), rid));
//
//         RID result = tree.search(Value(INT32_MAX));
//         REQUIRE(result.isValid());
//         REQUIRE(result.page_id == 1);
//     }
//
//     SECTION("Duplicate key insertion") {
//         RID rid1(1, 10);
//         RID rid2(2, 20);
//
//         REQUIRE(tree.insert(Value(42), rid1));
//
//         // 重复键插入应该失败或覆盖，取决于实现
//         bool duplicate_result = tree.insert(Value(42), rid2);
//
//         // 至少应该有一个结果存在
//         RID result = tree.search(Value(42));
//         REQUIRE(result.isValid());
//     }
// }
//
// TEST_CASE("BPlusTree Sequential Operations", "[bplustree][sequential]") {
//     std::cout << "=== Starting Basic Operations Test ===" << std::endl;
//     auto pager = test_utils::create_test_pager();
//     engine::BPlusTree tree(pager, TEST_INVALID_PAGE_ID, TypeId::INTEGER);
//
//     SECTION("Sequential insertion and verification") {
//         const int seq_count = 100;
//
//         // 顺序插入
//         for (int i = 0; i < seq_count; ++i) {
//             RID rid(i % 10 + 1, i % 5 + 1);
//             REQUIRE(tree.insert(Value(i), rid));
//         }
//
//         // 顺序验证
//         for (int i = 0; i < seq_count; ++i) {
//             RID result = tree.search(Value(i));
//             REQUIRE(result.isValid());
//             REQUIRE(result.page_id == i % 10 + 1);
//             REQUIRE(result.slot_num == i % 5 + 1);
//         }
//     }
//
//     SECTION("Reverse sequential operations") {
//         const int rev_count = 100;
//
//         // 逆序插入
//         for (int i = rev_count - 1; i >= 0; --i) {
//             RID rid(i % 10 + 1, i % 5 + 1);
//             REQUIRE(tree.insert(Value(i), rid));
//         }
//
//         // 顺序验证
//         for (int i = 0; i < rev_count; ++i) {
//             RID result = tree.search(Value(i));
//             REQUIRE(result.isValid());
//             REQUIRE(result.page_id == i % 10 + 1);
//             REQUIRE(result.slot_num == i % 5 + 1);
//         }
//     }
// }
//
// TEST_CASE("BPlusTree Performance Test", "[bplustree][performance]") {
//     std::cout << "=== Starting Basic Operations Test ===" << std::endl;
//     auto pager = test_utils::create_test_pager();
//     engine::BPlusTree tree(pager, TEST_INVALID_PAGE_ID, TypeId::INTEGER);
//
//     SECTION("Large scale insertion") {
//         const int large_count = 1000;
//         auto test_data = test_utils::generate_test_data(large_count);
//
//         // 批量插入
//         for (const auto& [key, rid] : test_data) {
//             REQUIRE(tree.insert(key, rid));
//         }
//
//         // 验证所有数据
//         for (const auto& [key, rid] : test_data) {
//             RID result = tree.search(key);
//             REQUIRE(result.isValid());
//             REQUIRE(result.page_id == rid.page_id);
//             REQUIRE(result.slot_num == rid.slot_num);
//         }
//
//         std::cout << "Large scale test - Height: " << tree.get_height()
//                   << ", Nodes: " << tree.get_node_count() << std::endl;
//     }
// }
//
// TEST_CASE("BPlusTree String Keys", "[bplustree][string]") {
//     std::cout << "=== Starting Basic Operations Test ===" << std::endl;
//     auto pager = test_utils::create_test_pager();
//     engine::BPlusTree tree(pager, TEST_INVALID_PAGE_ID, TypeId::VARCHAR);
//
//     SECTION("String key operations") {
//         RID rid1(1, 1);
//         RID rid2(2, 2);
//         RID rid3(3, 3);
//
//         REQUIRE(tree.insert(Value("apple"), rid1));
//         REQUIRE(tree.insert(Value("banana"), rid2));
//         REQUIRE(tree.insert(Value("cherry"), rid3));
//
//         RID result1 = tree.search(Value("apple"));
//         REQUIRE(result1.isValid());
//         REQUIRE(result1.page_id == 1);
//
//         RID result2 = tree.search(Value("banana"));
//         REQUIRE(result2.isValid());
//         REQUIRE(result2.page_id == 2);
//
//         RID result3 = tree.search(Value("cherry"));
//         REQUIRE(result3.isValid());
//         REQUIRE(result3.page_id == 3);
//
//         // 搜索不存在的字符串键
//         REQUIRE(tree.search(Value("date")).isValid() == false);
//     }
// }
//
// // 测试完成后清理文件
// struct TestCleanup {
//     ~TestCleanup() {
//         if (std::filesystem::exists(TEST_DB_FILE)) {
//             std::filesystem::remove(TEST_DB_FILE);
//         }
//     }
// };
//
// TestCleanup global_cleanup;