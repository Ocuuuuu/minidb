#include <../tests/catch2/catch_amalgamated.hpp>
#include <engine/bPlusTree/bplus_tree_page.h>
#include <storage/Page.h>
#include <common/Value.h>
#include <common/Types.h>
#include <memory>
#include <vector>
#include <cstring>

TEST_CASE("BPlusTreePage basic functionality", "[bplustreepage][unit]")
{
    // 创建测试用的页数据
    auto page = std::make_unique<minidb::storage::Page>(1);

    SECTION("BPlusTreePage construction and header initialization") {
        minidb::engine::BPlusTreePage bpage(page.get());

        REQUIRE(bpage.is_leaf() == false); // 默认应该是内部节点
        REQUIRE(bpage.get_key_count() == 0);
        REQUIRE(bpage.get_next_page_id() == 0);
        REQUIRE(bpage.get_key_type() == minidb::TypeId::INTEGER); // 默认类型
    }

    SECTION("Leaf node type management") {
        minidb::engine::BPlusTreePage bpage(page.get());

        bpage.set_leaf(true);
        REQUIRE(bpage.is_leaf() == true);

        bpage.set_leaf(false);
        REQUIRE(bpage.is_leaf() == false);
    }

    SECTION("Key type management") {
        minidb::engine::BPlusTreePage bpage(page.get());

        bpage.set_key_type(minidb::TypeId::INTEGER);
        REQUIRE(bpage.get_key_type() == minidb::TypeId::INTEGER);

        bpage.set_key_type(minidb::TypeId::BOOLEAN);
        REQUIRE(bpage.get_key_type() == minidb::TypeId::BOOLEAN);

        bpage.set_key_type(minidb::TypeId::VARCHAR);
        REQUIRE(bpage.get_key_type() == minidb::TypeId::VARCHAR);
    }
}

TEST_CASE("BPlusTreePage key operations", "[bplustreepage][key][unit]")
{
    auto page = std::make_unique<minidb::storage::Page>(1);
    minidb::engine::BPlusTreePage bpage(page.get());

    bpage.set_key_type(minidb::TypeId::INTEGER);
    bpage.set_key_count(3);

    SECTION("Set and get integer keys") {
        minidb::Value key1(42);
        minidb::Value key2(100);
        minidb::Value key3(-5);

        REQUIRE_NOTHROW(bpage.set_key_at(0, key1));
        REQUIRE_NOTHROW(bpage.set_key_at(1, key2));
        REQUIRE_NOTHROW(bpage.set_key_at(2, key3));

        REQUIRE(bpage.get_key_at(0).getAsInt() == 42);
        REQUIRE(bpage.get_key_at(1).getAsInt() == 100);
        REQUIRE(bpage.get_key_at(2).getAsInt() == -5);
    }

    SECTION("Key index out of range handling") {
        minidb::Value key(42);

        REQUIRE_THROWS_AS(bpage.set_key_at(5, key), std::out_of_range);
        REQUIRE_THROWS_AS(bpage.get_key_at(5), std::out_of_range);
    }

    SECTION("Remove keys") {
        minidb::Value key1(42);
        minidb::Value key2(100);
        minidb::Value key3(-5);

        bpage.set_key_at(0, key1);
        bpage.set_key_at(1, key2);
        bpage.set_key_at(2, key3);

        REQUIRE_NOTHROW(bpage.remove_key_at(1));
        REQUIRE(bpage.get_key_count() == 2);
        REQUIRE(bpage.get_key_at(0).getAsInt() == 42);
        REQUIRE(bpage.get_key_at(1).getAsInt() == -5);
    }
}

TEST_CASE("BPlusTreePage RID operations for leaf nodes", "[bplustreepage][rid][unit]")
{
    auto page = std::make_unique<minidb::storage::Page>(1);
    minidb::engine::BPlusTreePage bpage(page.get());

    bpage.set_leaf(true);
    bpage.set_key_type(minidb::TypeId::INTEGER);
    bpage.set_key_count(2);

    SECTION("Set and get RIDs in leaf node") {
        minidb::RID rid1{1, 10};
        minidb::RID rid2{2, 20};

        REQUIRE_NOTHROW(bpage.set_rid_at(0, rid1));
        REQUIRE_NOTHROW(bpage.set_rid_at(1, rid2));

        // 使用正确的RID字段名
        REQUIRE(bpage.get_rid_at(0).page_id == 1);
        REQUIRE(bpage.get_rid_at(0).slot_num == 10);
        REQUIRE(bpage.get_rid_at(1).page_id == 2);
        REQUIRE(bpage.get_rid_at(1).slot_num == 20);
    }

    SECTION("RID operations in non-leaf node should fail") {
        auto page2 = std::make_unique<minidb::storage::Page>(2);
        minidb::engine::BPlusTreePage bpage2(page2.get());

        bpage2.set_leaf(false); // 内部节点
        minidb::RID rid{1, 10};

        REQUIRE_THROWS_AS(bpage2.set_rid_at(0, rid), std::logic_error);
        REQUIRE_THROWS_AS(bpage2.get_rid_at(0), std::logic_error);
    }
}

TEST_CASE("BPlusTreePage child page operations for internal nodes", "[bplustreepage][child][unit]")
{
    auto page = std::make_unique<minidb::storage::Page>(1);
    minidb::engine::BPlusTreePage bpage(page.get());

    bpage.set_leaf(false);
    bpage.set_key_type(minidb::TypeId::INTEGER);
    bpage.set_key_count(2);

    SECTION("Set and get child page IDs in internal node") {
        REQUIRE_NOTHROW(bpage.set_child_page_id_at(0, 100));
        REQUIRE_NOTHROW(bpage.set_child_page_id_at(1, 200));
        REQUIRE_NOTHROW(bpage.set_child_page_id_at(2, 300));

        REQUIRE(bpage.get_child_page_id_at(0) == 100);
        REQUIRE(bpage.get_child_page_id_at(1) == 200);
        REQUIRE(bpage.get_child_page_id_at(2) == 300);
    }

    SECTION("Child page operations in leaf node should fail") {
        auto page2 = std::make_unique<minidb::storage::Page>(2);
        minidb::engine::BPlusTreePage bpage2(page2.get());

        bpage2.set_leaf(true); // 叶子节点

        REQUIRE_THROWS_AS(bpage2.set_child_page_id_at(0, 100), std::logic_error);
        REQUIRE_THROWS_AS(bpage2.get_child_page_id_at(0), std::logic_error);
    }
}

TEST_CASE("BPlusTreePage key search functionality", "[bplustreepage][search][unit]")
{
    auto page = std::make_unique<minidb::storage::Page>(1);
    minidb::engine::BPlusTreePage bpage(page.get());

    bpage.set_key_type(minidb::TypeId::INTEGER);
    bpage.set_key_count(5);

    // 设置有序键值
    std::vector<int> keys = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; ++i) {
        bpage.set_key_at(i, minidb::Value(keys[i]));
    }

    SECTION("Find existing keys") {
        REQUIRE(bpage.find_key_index(minidb::Value(10)) == 0);
        REQUIRE(bpage.find_key_index(minidb::Value(30)) == 2);
        REQUIRE(bpage.find_key_index(minidb::Value(50)) == 4);
    }

    SECTION("Find non-existing keys returns insertion hint") {
        int result = bpage.find_key_index(minidb::Value(5));
        REQUIRE(result == -1); // 应该插入在位置0

        result = bpage.find_key_index(minidb::Value(25));
        REQUIRE(result == -3); // 应该插入在位置2

        result = bpage.find_key_index(minidb::Value(60));
        REQUIRE(result == -6); // 应该插入在位置5
    }
}

TEST_CASE("BPlusTreePage insertion operations", "[bplustreepage][insert][unit]")
{
    auto page = std::make_unique<minidb::storage::Page>(1);
    minidb::engine::BPlusTreePage bpage(page.get());

    bpage.set_leaf(true);
    bpage.set_key_type(minidb::TypeId::INTEGER);

    SECTION("Insert leaf pairs") {
        minidb::RID rid1{1, 10};
        minidb::RID rid2{1, 20};
        minidb::RID rid3{1, 30};

        REQUIRE(bpage.insert_leaf_pair(minidb::Value(30), rid1));
        REQUIRE(bpage.insert_leaf_pair(minidb::Value(10), rid2));
        REQUIRE(bpage.insert_leaf_pair(minidb::Value(20), rid3));

        REQUIRE(bpage.get_key_count() == 3);
        REQUIRE(bpage.get_key_at(0).getAsInt() == 10);
        REQUIRE(bpage.get_key_at(1).getAsInt() == 20);
        REQUIRE(bpage.get_key_at(2).getAsInt() == 30);

        REQUIRE(bpage.get_rid_at(0).slot_num == 20);
        REQUIRE(bpage.get_rid_at(1).slot_num == 30);
        REQUIRE(bpage.get_rid_at(2).slot_num == 10);
    }

    SECTION("Insert internal pairs") {
        auto page2 = std::make_unique<minidb::storage::Page>(2);
        minidb::engine::BPlusTreePage bpage2(page2.get());

        bpage2.set_leaf(false);
        bpage2.set_key_type(minidb::TypeId::INTEGER);

        REQUIRE(bpage2.insert_internal_pair(minidb::Value(30), 300));
        REQUIRE(bpage2.insert_internal_pair(minidb::Value(10), 100));
        REQUIRE(bpage2.insert_internal_pair(minidb::Value(20), 200));

        REQUIRE(bpage2.get_key_count() == 3);
        REQUIRE(bpage2.get_key_at(0).getAsInt() == 10);
        REQUIRE(bpage2.get_key_at(1).getAsInt() == 20);
        REQUIRE(bpage2.get_key_at(2).getAsInt() == 30);

        // 内部节点有n+1个子指针，第一个指针应该是初始值0
        REQUIRE(bpage2.get_child_page_id_at(0) == 0); // 修改这里：0 == 0
        REQUIRE(bpage2.get_child_page_id_at(1) == 100);
        REQUIRE(bpage2.get_child_page_id_at(2) == 200);
        REQUIRE(bpage2.get_child_page_id_at(3) == 300); // 修改这里：300 == 300
    }
}

TEST_CASE("BPlusTreePage capacity management", "[bplustreepage][capacity][unit]")
{
    auto page = std::make_unique<minidb::storage::Page>(1);
    minidb::engine::BPlusTreePage bpage(page.get());

    bpage.set_leaf(true);
    bpage.set_key_type(minidb::TypeId::INTEGER);

    SECTION("Capacity calculations") {
        uint16_t max_capacity = bpage.get_max_capacity();
        REQUIRE(max_capacity > 0);

        REQUIRE(bpage.is_full() == false);
        REQUIRE(bpage.is_underflow() == true); // 空节点应该下溢

        uint16_t free_space = bpage.get_free_space();
        REQUIRE(free_space > 0);
    }

    SECTION("Full and underflow detection") {
        uint16_t max_cap = bpage.get_max_capacity();
        REQUIRE(max_cap > 1); // 确保能插入

        // 插入直到满
        for (int i = 0; i < max_cap; ++i) {
            REQUIRE(bpage.insert_leaf_pair(minidb::Value(i), minidb::RID{1, i}));
        }
        REQUIRE(bpage.is_full());
        REQUIRE_FALSE(bpage.is_underflow());

        // 删除到下溢：保留不到一半
        uint16_t min_capacity = max_cap / 2;
        uint16_t target = min_capacity - 1; // 下溢阈值
        while (bpage.get_key_count() > target) {
            REQUIRE(bpage.remove_leaf_pair(0));
        }

        REQUIRE_FALSE(bpage.is_full());
        REQUIRE(bpage.is_underflow()); // ✅ 此时才应下溢
    }
}

TEST_CASE("BPlusTreePage edge cases", "[bplustreepage][edge][unit]")
{
    SECTION("Empty page operations") {
        auto page = std::make_unique<minidb::storage::Page>(1);
        minidb::engine::BPlusTreePage bpage(page.get());

        REQUIRE(bpage.get_key_count() == 0);
        REQUIRE(bpage.is_leaf() == false);
        REQUIRE(bpage.get_next_page_id() == 0);
    }

    SECTION("Single element operations") {
        auto page = std::make_unique<minidb::storage::Page>(1);
        minidb::engine::BPlusTreePage bpage(page.get());

        bpage.set_leaf(true);
        bpage.set_key_type(minidb::TypeId::INTEGER);

        minidb::RID rid{1, 1};
        REQUIRE(bpage.insert_leaf_pair(minidb::Value(42), rid));

        REQUIRE(bpage.get_key_count() == 1);
        REQUIRE(bpage.get_key_at(0).getAsInt() == 42);
        REQUIRE(bpage.get_rid_at(0).slot_num == 1);

        REQUIRE(bpage.remove_leaf_pair(0));
        REQUIRE(bpage.get_key_count() == 0);
    }

    SECTION("Different key types") {
        auto page = std::make_unique<minidb::storage::Page>(1);
        minidb::engine::BPlusTreePage bpage(page.get());

        bpage.set_leaf(true);

        // 测试布尔类型
        bpage.set_key_type(minidb::TypeId::BOOLEAN);
        minidb::RID rid1{1, 1};
        REQUIRE(bpage.insert_leaf_pair(minidb::Value(true), rid1));
        REQUIRE(bpage.get_key_at(0).getAsBool() == true);

        // 测试字符串类型
        auto page2 = std::make_unique<minidb::storage::Page>(2);
        minidb::engine::BPlusTreePage bpage2(page2.get());

        bpage2.set_leaf(true);
        bpage2.set_key_type(minidb::TypeId::VARCHAR);
        minidb::RID rid2{1, 2};
        REQUIRE(bpage2.insert_leaf_pair(minidb::Value("test"), rid2));
        REQUIRE(bpage2.get_key_at(0).getAsString() == "test");
    }
}

TEST_CASE("BPlusTreePage serialization integrity", "[bplustreepage][serialization][unit]")
{
    auto page = std::make_unique<minidb::storage::Page>(1);
    minidb::engine::BPlusTreePage bpage(page.get());

    bpage.set_leaf(true);
    bpage.set_key_type(minidb::TypeId::INTEGER);
    bpage.set_next_page_id(123);

    SECTION("Header serialization preservation") {
        bpage.save_header();

        // 重新创建页面应该保留头信息
        minidb::engine::BPlusTreePage bpage2(page.get());
        REQUIRE(bpage2.is_leaf() == true);
        REQUIRE(bpage2.get_key_type() == minidb::TypeId::INTEGER);
        REQUIRE(bpage2.get_next_page_id() == 123);
    }

    SECTION("Data serialization preservation") {
        minidb::RID rid1{1, 10};
        minidb::RID rid2{1, 20};
        bpage.insert_leaf_pair(minidb::Value(42), rid1);
        bpage.insert_leaf_pair(minidb::Value(84), rid2);

        // 重新创建页面应该保留数据
        minidb::engine::BPlusTreePage bpage2(page.get());
        REQUIRE(bpage2.get_key_count() == 2);
        REQUIRE(bpage2.get_key_at(0).getAsInt() == 42);
        REQUIRE(bpage2.get_key_at(1).getAsInt() == 84);
        REQUIRE(bpage2.get_rid_at(0).slot_num == 10);
        REQUIRE(bpage2.get_rid_at(1).slot_num == 20);
    }
}