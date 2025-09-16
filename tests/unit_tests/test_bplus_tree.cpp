#include <../tests/catch2/catch_amalgamated.hpp>
#include "storage/Pager.h"
#include "engine/bPlusTree/bplus_tree.h"
#include "common/Value.h"
#include "common/Types.h"
#include "common/Exception.h"
#include "storage/BufferManager.h"
#include "storage/DiskManager.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include <filesystem>

using namespace minidb;

// 测试配置
constexpr const char* TEST_DB_FILE = "test_bplus_tree_v2.db";
constexpr int TEST_PAGE_SIZE = 4096;
constexpr PageID TEST_INVALID_PAGE_ID = -1;

// 测试辅助函数
namespace test_utils {

    // 清理测试文件（带错误处理）
    void cleanup_test_files(const std::string& db_filename) {
        std::string minidb_file = db_filename + ".minidb";

        try {
            if (std::filesystem::exists(db_filename)) {
                std::filesystem::remove(db_filename);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Warning: Could not remove " << db_filename << ": " << e.what() << std::endl;
        }

        try {
            if (std::filesystem::exists(minidb_file)) {
                std::filesystem::remove(minidb_file);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Warning: Could not remove " << minidb_file << ": " << e.what() << std::endl;
        }
    }


    // 创建测试用的Pager（需要显式指定文件名）
    std::shared_ptr<storage::Pager> create_test_pager(const std::string& db_filename) {
        // 先清理旧测试文件
        cleanup_test_files(db_filename);

        // 创建FileManager、DiskManager和BufferManager
        auto file_manager = std::make_shared<storage::FileManager>();
        file_manager->createDatabase(db_filename);
        file_manager->openDatabase(db_filename);

        auto disk_manager = std::make_shared<storage::DiskManager>(file_manager);
        auto buffer_manager = std::make_shared<storage::BufferManager>(disk_manager, 1000);

        return std::make_shared<storage::Pager>(disk_manager, buffer_manager);
    }

    // 生成测试数据
    std::vector<std::pair<Value, RID>> generate_test_data(int count, int start = 0) {
        std::vector<std::pair<Value, RID>> data;
        data.reserve(count);

        for (int i = start; i < start + count; ++i) {
            data.emplace_back(Value(i), RID{i % 10 + 1, i % 100 + 1});
        }
        return data;
    }

    // 生成随机测试数据
    std::vector<std::pair<Value, RID>> generate_random_test_data(int count) {
        std::vector<std::pair<Value, RID>> data;
        data.reserve(count);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, count * 2);

        for (int i = 0; i < count; ++i) {
            int key = distrib(gen);
            data.emplace_back(Value(key), RID{key % 10 + 1, key % 100 + 1});
        }
        return data;
    }

    // 生成字符串测试数据
    std::vector<std::pair<Value, RID>> generate_string_test_data(int count) {
        std::vector<std::pair<Value, RID>> data;
        data.reserve(count);

        const char* fruits[] = {"apple", "banana", "cherry", "date", "elderberry",
                               "fig", "grape", "honeydew", "kiwi", "lemon"};

        for (int i = 0; i < count; ++i) {
            std::string key_str = fruits[i % 10] + std::to_string(i / 10);
            data.emplace_back(Value(key_str), RID{i % 10 + 1, i % 100 + 1});
        }
        return data;
    }

    struct TestPager {
        std::shared_ptr<storage::Pager> pager;
        std::string db_filename;

        TestPager(const std::string& filename)
            : db_filename(filename), pager(create_test_pager(filename)) {}

        ~TestPager() {
            cleanup_test_files(db_filename);
        }

        std::shared_ptr<storage::Pager> get() const { return pager; }
        operator std::shared_ptr<storage::Pager>() const { return pager; }
    };

} // namespace test_utils

// 修改所有测试用例，使用 TestPager 替代 create_test_pager
TEST_CASE("BPlusTree Basic Operations - Enhanced", "[bplustree][basic]") {
    std::cout << "=== Starting Enhanced Basic Operations Test ===" << std::endl;

    try {
        test_utils::TestPager pager_wrapper("test_basic_ops.db");
        std::cout << "Pager created successfully" << std::endl;

        engine::BPlusTree tree(pager_wrapper, TEST_INVALID_PAGE_ID, TypeId::INTEGER);
        std::cout << "BPlusTree created successfully" << std::endl;

        SECTION("Empty tree operations") {
            std::cout << "Testing empty tree operations..." << std::endl;
            REQUIRE(tree.search(Value(42)).isValid() == false);
            std::cout << "Search test passed" << std::endl;

            REQUIRE(tree.get_height() == 0);
            std::cout << "Height test passed" << std::endl;

            REQUIRE(tree.get_node_count() == 0);
            std::cout << "Node count test passed" << std::endl;

            // 测试空树的范围查询
            auto results = tree.range_search(Value(0), Value(100));
            REQUIRE(results.empty());
            std::cout << "Range search test passed" << std::endl;
        }

        SECTION("Single insertion and search") {
            RID rid1(1, 10);
            REQUIRE(tree.insert(Value(42), rid1));

            RID result = tree.search(Value(42));
            REQUIRE(result.isValid());
            REQUIRE(result.page_id == 1);
            REQUIRE(result.slot_num == 10);

            // 搜索不存在的键
            REQUIRE(tree.search(Value(999)).isValid() == false);

            // 测试树的高度和节点数
            REQUIRE(tree.get_height() == 1);
            REQUIRE(tree.get_node_count() == 1);
        }

        SECTION("Multiple insertions and searches with verification") {
            const int test_count = 20;
            auto test_data = test_utils::generate_test_data(test_count);

            // 插入所有键值对
            for (size_t i = 0; i < test_data.size(); ++i) {
                REQUIRE(tree.insert(test_data[i].first, test_data[i].second));

                // 验证插入后立即可以搜索到
                RID result = tree.search(test_data[i].first);
                REQUIRE(result.isValid());
                REQUIRE(result.page_id == test_data[i].second.page_id);
                REQUIRE(result.slot_num == test_data[i].second.slot_num);
            }

            // 验证所有键都能找到
            for (size_t i = 0; i < test_data.size(); ++i) {
                RID result = tree.search(test_data[i].first);
                REQUIRE(result.isValid());
                REQUIRE(result.page_id == test_data[i].second.page_id);
                REQUIRE(result.slot_num == test_data[i].second.slot_num);
            }

            // 验证树的结构
            REQUIRE(tree.get_height() >= 1);
            REQUIRE(tree.get_node_count() >= 1);

            std::cout << "After " << test_count << " inserts - Height: "
                      << tree.get_height() << ", Nodes: " << tree.get_node_count() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        throw;
    }
}

TEST_CASE("BPlusTree Node Splitting and Structure", "[bplustree][splitting]") {
    std::cout << "=== Starting Node Splitting Test ===" << std::endl;
    test_utils::TestPager pager_wrapper("test_node_ops.db");
    engine::BPlusTree tree(pager_wrapper, TEST_INVALID_PAGE_ID, TypeId::INTEGER);

    SECTION("Leaf node splitting") {
        // 插入足够的数据以触发叶子节点分裂
        const int test_count = 100;
        auto test_data = test_utils::generate_test_data(test_count);

        uint32_t initial_height = tree.get_height();
        uint32_t initial_nodes = tree.get_node_count();

        for (const auto& [key, rid] : test_data) {
            REQUIRE(tree.insert(key, rid));
        }

        // 验证树的高度和节点数增加了
        REQUIRE(tree.get_height() > initial_height);
        REQUIRE(tree.get_node_count() > initial_nodes);
        REQUIRE(tree.get_node_count() >= 3); // 至少根节点和两个叶子节点

        std::cout << "After splitting - Height: " << tree.get_height()
                  << ", Nodes: " << tree.get_node_count() << std::endl;

        // 验证所有数据都能正确检索
        for (const auto& [key, rid] : test_data) {
            RID result = tree.search(key);
            REQUIRE(result.isValid());
            REQUIRE(result.page_id == rid.page_id);
            REQUIRE(result.slot_num == rid.slot_num);
        }
    }

    SECTION("Internal node splitting") {
        // 插入更多数据以触发内部节点分裂
        const int test_count = 500;
        auto test_data = test_utils::generate_test_data(test_count);

        for (const auto& [key, rid] : test_data) {
            REQUIRE(tree.insert(key, rid));
        }

        // 验证树的结构
        REQUIRE(tree.get_height() >= 2);
        REQUIRE(tree.get_node_count() >= 5);

        std::cout << "After internal splitting - Height: " << tree.get_height()
                  << ", Nodes: " << tree.get_node_count() << std::endl;

        // 随机抽样验证
        std::vector<int> test_indices = {0, 50, 100, 200, 300, 400, 499};
        for (int idx : test_indices) {
            RID result = tree.search(test_data[idx].first);
            REQUIRE(result.isValid());
            REQUIRE(result.page_id == test_data[idx].second.page_id);
        }
    }
}

TEST_CASE("BPlusTree Range Search - Enhanced", "[bplustree][range]") {
    std::cout << "=== Starting Enhanced Range Search Test ===" << std::endl;
    test_utils::TestPager pager_wrapper("test_enhanced_ops.db");
    engine::BPlusTree tree(pager_wrapper, TEST_INVALID_PAGE_ID, TypeId::INTEGER);

    SECTION("Basic range search with exact bounds") {
        // 插入有序数据
        for (int i = 0; i < 50; ++i) {
            REQUIRE(tree.insert(Value(i), RID{i, i * 10}));
        }

        // 范围查询 [10, 30]
        auto results = tree.range_search(Value(10), Value(30));
        REQUIRE(results.size() == 21); // 10到30 inclusive

        // 验证结果顺序和正确性
        for (int i = 0; i < results.size(); ++i) {
            REQUIRE(results[i].page_id == 10 + i);
            REQUIRE(results[i].slot_num == (10 + i) * 10);
        }
    }

    SECTION("Range search with non-existent bounds") {
        for (int i = 0; i < 20; ++i) {
            REQUIRE(tree.insert(Value(i * 2), RID{i * 2, i})); // 只插入偶数
        }

        // 查询奇数范围
        auto results = tree.range_search(Value(1), Value(39));
        REQUIRE(results.size() == 0);
    }

    SECTION("Large range search across multiple leaves") {
        // 插入大量数据以确保跨多个叶子节点
        for (int i = 0; i < 200; ++i) {
            REQUIRE(tree.insert(Value(i), RID{i, i}));
        }

        // 大范围查询
        auto results = tree.range_search(Value(50), Value(150));
        REQUIRE(results.size() == 101); // 50到150 inclusive

        // 验证所有结果都在范围内
        for (const auto& rid : results) {
            REQUIRE(rid.page_id >= 50);
            REQUIRE(rid.page_id <= 150);
        }
    }
}

TEST_CASE("BPlusTree Edge Cases and Error Handling", "[bplustree][edge]") {
    std::cout << "=== Starting Edge Cases Test ===" << std::endl;
    test_utils::TestPager pager_wrapper("test_edge_ops.db");
    engine::BPlusTree tree(pager_wrapper, TEST_INVALID_PAGE_ID, TypeId::INTEGER);

    SECTION("Extreme value keys") {
        RID rid_min(1, 1);
        RID rid_max(2, 2);

        // 测试最小和最大整数值
        REQUIRE(tree.insert(Value(INT32_MIN), rid_min));
        REQUIRE(tree.insert(Value(INT32_MAX), rid_max));

        RID result_min = tree.search(Value(INT32_MIN));
        REQUIRE(result_min.isValid());
        REQUIRE(result_min.page_id == 1);

        RID result_max = tree.search(Value(INT32_MAX));
        REQUIRE(result_max.isValid());
        REQUIRE(result_max.page_id == 2);
    }

    SECTION("Duplicate key handling") {
        RID rid1(1, 10);
        RID rid2(2, 20);

        REQUIRE(tree.insert(Value(42), rid1));

        // 重复键插入 - 行为取决于实现（可能覆盖或失败）
        bool duplicate_result = tree.insert(Value(42), rid2);

        // 至少应该有一个结果存在
        RID result = tree.search(Value(42));
        REQUIRE(result.isValid());
    }

    SECTION("Type validation") {
        RID rid(1, 1);

        // 测试类型不匹配应该抛出异常
        REQUIRE_THROWS_AS(tree.insert(Value("string_key"), rid), std::runtime_error);
        REQUIRE_THROWS_AS(tree.search(Value("string_key")), std::runtime_error);
    }
}

TEST_CASE("BPlusTree Sequential and Reverse Operations", "[bplustree][sequential]") {
    std::cout << "=== Starting Sequential Operations Test ===" << std::endl;
    test_utils::TestPager pager_wrapper("test_sequential_ops.db");
    engine::BPlusTree tree(pager_wrapper, TEST_INVALID_PAGE_ID, TypeId::INTEGER);

    SECTION("Large sequential insertion") {
        const int seq_count = 1000;

        // 顺序插入大量数据
        for (int i = 0; i < seq_count; ++i) {
            RID rid(i % 10 + 1, i % 5 + 1);
            REQUIRE(tree.insert(Value(i), rid));
        }

        // 顺序验证所有数据
        for (int i = 0; i < seq_count; ++i) {
            RID result = tree.search(Value(i));
            REQUIRE(result.isValid());
            REQUIRE(result.page_id == i % 10 + 1);
            REQUIRE(result.slot_num == i % 5 + 1);
        }

        std::cout << "Large sequential test - Height: " << tree.get_height()
                  << ", Nodes: " << tree.get_node_count() << std::endl;
    }

    SECTION("Reverse sequential operations") {
        const int rev_count = 500;

        // 逆序插入
        for (int i = rev_count - 1; i >= 0; --i) {
            RID rid(i % 8 + 1, i % 6 + 1);
            REQUIRE(tree.insert(Value(i), rid));
        }

        // 顺序验证
        for (int i = 0; i < rev_count; ++i) {
            RID result = tree.search(Value(i));
            REQUIRE(result.isValid());
            REQUIRE(result.page_id == i % 8 + 1);
            REQUIRE(result.slot_num == i % 6 + 1);
        }
    }
}

TEST_CASE("BPlusTree String Key Operations", "[bplustree][string]") {
    std::cout << "=== Starting String Key Test ===" << std::endl;
    test_utils::TestPager pager_wrapper("test_string_ops.db");
    engine::BPlusTree tree(pager_wrapper, TEST_INVALID_PAGE_ID, TypeId::VARCHAR);

    SECTION("Basic string key operations") {
        RID rid1(1, 1);
        RID rid2(2, 2);
        RID rid3(3, 3);

        REQUIRE(tree.insert(Value("apple"), rid1));
        REQUIRE(tree.insert(Value("banana"), rid2));
        REQUIRE(tree.insert(Value("cherry"), rid3));

        RID result1 = tree.search(Value("apple"));
        REQUIRE(result1.isValid());
        REQUIRE(result1.page_id == 1);

        RID result2 = tree.search(Value("banana"));
        REQUIRE(result2.isValid());
        REQUIRE(result2.page_id == 2);

        RID result3 = tree.search(Value("cherry"));
        REQUIRE(result3.isValid());
        REQUIRE(result3.page_id == 3);

        // 测试字符串比较和排序
        auto results = tree.range_search(Value("apple"), Value("cherry"));
        REQUIRE(results.size() == 3);
    }

    SECTION("Multiple string keys with splitting") {
        auto test_data = test_utils::generate_string_test_data(100);

        for (const auto& [key, rid] : test_data) {
            REQUIRE(tree.insert(key, rid));
        }

        // 验证所有字符串键都能找到
        for (const auto& [key, rid] : test_data) {
            RID result = tree.search(key);
            REQUIRE(result.isValid());
            REQUIRE(result.page_id == rid.page_id);
        }

        std::cout << "String key test - Height: " << tree.get_height()
                  << ", Nodes: " << tree.get_node_count() << std::endl;
    }
}

TEST_CASE("BPlusTree Mixed Operations", "[bplustree][mixed]") {
    std::cout << "=== Starting Mixed Operations Test ===" << std::endl;
    test_utils::TestPager pager_wrapper("test_mixed_ops.db");
    engine::BPlusTree tree(pager_wrapper, TEST_INVALID_PAGE_ID, TypeId::INTEGER);

    SECTION("Interleaved insertions and searches") {
        const int operation_count = 200;

        for (int i = 0; i < operation_count; ++i) {
            if (i % 3 == 0) {
                // 每3次操作插入一次
                RID rid(i % 5 + 1, i % 10 + 1);
                REQUIRE(tree.insert(Value(i), rid));

                // 插入后立即验证
                RID result = tree.search(Value(i));
                REQUIRE(result.isValid());
            } else if (i % 3 == 1) {
                // 搜索已存在的键（如果存在）
                if (i > 0) {
                    RID result = tree.search(Value(i - 1));
                    // 可能找到也可能找不到，都正常
                }
            }
            // else: 什么都不做
        }
    }

    SECTION("Random operations stress test") {
        const int test_count = 300;
        auto random_data = test_utils::generate_random_test_data(test_count);

        // 插入所有随机数据
        for (const auto& [key, rid] : random_data) {
            REQUIRE(tree.insert(key, rid));
        }

        // 随机抽样验证
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, test_count - 1);

        for (int i = 0; i < 50; ++i) {
            int idx = distrib(gen);
            RID result = tree.search(random_data[idx].first);
            REQUIRE(result.isValid());
            REQUIRE(result.page_id == random_data[idx].second.page_id);
        }
    }
}

TEST_CASE("BPlusTree Tree Structure Validation", "[bplustree][structure]") {
    std::cout << "=== Starting Tree Structure Test ===" << std::endl;
    test_utils::TestPager pager_wrapper("test_tree_ops.db");
    engine::BPlusTree tree(pager_wrapper, TEST_INVALID_PAGE_ID, TypeId::INTEGER);

    SECTION("Tree growth validation") {
        const int step_count = 10;
        const int step_size = 50;

        std::vector<uint32_t> heights;
        std::vector<uint32_t> node_counts;

        for (int step = 0; step < step_count; ++step) {
            int start = step * step_size;
            auto test_data = test_utils::generate_test_data(step_size, start);

            for (const auto& [key, rid] : test_data) {
                REQUIRE(tree.insert(key, rid));
            }

            heights.push_back(tree.get_height());
            node_counts.push_back(tree.get_node_count());

            std::cout << "Step " << step << ": Height=" << heights.back()
                      << ", Nodes=" << node_counts.back() << std::endl;
        }

        // 验证树在增长（高度可能增加）
        for (size_t i = 1; i < heights.size(); ++i) {
            REQUIRE(heights[i] >= heights[i - 1]);
            REQUIRE(node_counts[i] > node_counts[i - 1]);
        }
    }

    SECTION("Tree printing for debugging") {
        // 插入一些数据然后打印树结构
        for (int i = 0; i < 30; ++i) {
            REQUIRE(tree.insert(Value(i), RID{i, i}));
        }

        // 打印树结构（用于调试）
        std::cout << "Tree structure:" << std::endl;
        tree.print_tree();
    }
}

