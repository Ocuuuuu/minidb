#include <../tests/catch2/catch_amalgamated.hpp>
#include <engine/catalog/catalog_manager.h>
#include <engine/catalog/schema.h>
#include <stdexcept>

TEST_CASE("CatalogManager basic functionality", "[catalog][manager][unit]")
{
    minidb::CatalogManager catalog;
    
    // 创建测试用的列定义
    std::vector<minidb::Column> columns = {
        minidb::Column{"id", minidb::TypeId::INTEGER, 4, 0},
        minidb::Column{"name", minidb::TypeId::VARCHAR, 50, 0}
    };
    
    minidb::Schema schema(columns);

    SECTION("Create table operations") {
        // 测试创建新表
        REQUIRE(catalog.create_table("users", schema) == true);
        REQUIRE(catalog.table_exists("users") == true);
        REQUIRE(catalog.get_table_count() == 1);

        // 测试重复创建表
        REQUIRE(catalog.create_table("users", schema) == false);
        REQUIRE(catalog.get_table_count() == 1);

        // 创建第二个表
        REQUIRE(catalog.create_table("products", schema) == true);
        REQUIRE(catalog.table_exists("products") == true);
        REQUIRE(catalog.get_table_count() == 2);
    }

    SECTION("Drop table operations") {
        // 先创建表
        REQUIRE(catalog.create_table("users", schema) == true);
        
        // 测试删除存在的表
        REQUIRE(catalog.drop_table("users") == true);
        REQUIRE(catalog.table_exists("users") == false);
        REQUIRE(catalog.get_table_count() == 0);

        // 测试删除不存在的表
        REQUIRE(catalog.drop_table("nonexistent") == false);
        REQUIRE(catalog.get_table_count() == 0);
    }

    SECTION("Table existence checking") {
        // 初始状态下表不存在
        REQUIRE(catalog.table_exists("users") == false);
        REQUIRE(catalog.table_exists("products") == false);

        // 创建表后检查存在性
        REQUIRE(catalog.create_table("users", schema) == true);
        REQUIRE(catalog.table_exists("users") == true);
        REQUIRE(catalog.table_exists("products") == false);
    }
}

TEST_CASE("CatalogManager table retrieval", "[catalog][retrieval][unit]")
{
    minidb::CatalogManager catalog;
    
    std::vector<minidb::Column> columns = {
        minidb::Column{"id", minidb::TypeId::INTEGER, 4, 0},
        minidb::Column{"name", minidb::TypeId::VARCHAR, 50, 0}
    };
    
    minidb::Schema schema(columns);

    SECTION("Get table pointer operations") {
        // 创建表
        REQUIRE(catalog.create_table("users", schema) == true);

        // 测试获取非常量表指针
        minidb::TableInfo* table_ptr = catalog.get_table("users");
        REQUIRE(table_ptr != nullptr);
        REQUIRE(table_ptr->get_table_name() == "users");
        REQUIRE(table_ptr->get_schema().get_column_count() == 2);

        // 测试获取常量表指针
        const minidb::TableInfo* const_table_ptr = catalog.get_table("users");
        REQUIRE(const_table_ptr != nullptr);
        REQUIRE(const_table_ptr->get_table_name() == "users");

        // 测试获取不存在的表
        REQUIRE(catalog.get_table("nonexistent") == nullptr);
        REQUIRE(catalog.get_table("") == nullptr);
    }

    SECTION("Const correctness") {
        // 创建表
        REQUIRE(catalog.create_table("users", schema) == true);

        // 测试常量版本的CatalogManager
        const minidb::CatalogManager& const_catalog = catalog;
        const minidb::TableInfo* const_table_ptr = const_catalog.get_table("users");
        REQUIRE(const_table_ptr != nullptr);
        REQUIRE(const_table_ptr->get_table_name() == "users");

        // 常量版本应该返回nullptr对于不存在的表
        REQUIRE(const_catalog.get_table("nonexistent") == nullptr);
    }
}

TEST_CASE("CatalogManager table enumeration", "[catalog][enumeration][unit]")
{
    minidb::CatalogManager catalog;
    
    std::vector<minidb::Column> columns = {
        minidb::Column{"id", minidb::TypeId::INTEGER, 4, 0}
    };
    
    minidb::Schema schema(columns);

    SECTION("Empty catalog operations") {
        REQUIRE(catalog.get_table_count() == 0);
        REQUIRE(catalog.get_table_names().empty() == true);
    }

    SECTION("Single table operations") {
        REQUIRE(catalog.create_table("users", schema) == true);
        
        auto table_names = catalog.get_table_names();
        REQUIRE(table_names.size() == 1);
        REQUIRE(table_names[0] == "users");
        REQUIRE(catalog.get_table_count() == 1);
    }

    SECTION("Multiple tables operations") {
        // 创建多个表
        REQUIRE(catalog.create_table("users", schema) == true);
        REQUIRE(catalog.create_table("products", schema) == true);
        REQUIRE(catalog.create_table("orders", schema) == true);
        
        // 验证表数量
        REQUIRE(catalog.get_table_count() == 3);
        
        // 验证表名列表（应该按字母顺序排序）
        auto table_names = catalog.get_table_names();
        REQUIRE(table_names.size() == 3);
        REQUIRE(table_names[0] == "orders");    // 字母顺序：orders, products, users
        REQUIRE(table_names[1] == "products");
        REQUIRE(table_names[2] == "users");
        
        // 验证表存在性
        REQUIRE(catalog.table_exists("users") == true);
        REQUIRE(catalog.table_exists("products") == true);
        REQUIRE(catalog.table_exists("orders") == true);
        REQUIRE(catalog.table_exists("nonexistent") == false);
    }

    SECTION("Table enumeration after deletion") {
        // 创建多个表
        REQUIRE(catalog.create_table("users", schema) == true);
        REQUIRE(catalog.create_table("products", schema) == true);
        REQUIRE(catalog.create_table("orders", schema) == true);
        
        // 删除一个表
        REQUIRE(catalog.drop_table("products") == true);
        
        // 验证剩余的表
        REQUIRE(catalog.get_table_count() == 2);
        
        auto table_names = catalog.get_table_names();
        REQUIRE(table_names.size() == 2);
        REQUIRE(table_names[0] == "orders");
        REQUIRE(table_names[1] == "users");
        
        // 验证表存在性
        REQUIRE(catalog.table_exists("users") == true);
        REQUIRE(catalog.table_exists("orders") == true);
        REQUIRE(catalog.table_exists("products") == false);
    }
}

TEST_CASE("CatalogManager edge cases", "[catalog][edge][unit]")
{
    minidb::CatalogManager catalog;
    
    std::vector<minidb::Column> columns = {
        minidb::Column{"id", minidb::TypeId::INTEGER, 4, 0}
    };
    
    minidb::Schema schema(columns);

    SECTION("Empty table name operations") {
        // 测试空表名
        REQUIRE(catalog.create_table("", schema) == true); // 假设允许空表名
        REQUIRE(catalog.table_exists("") == true);
        REQUIRE(catalog.get_table_count() == 1);
        
        // 测试获取空表名的表
        REQUIRE(catalog.get_table("") != nullptr);
        
        // 测试删除空表名的表
        REQUIRE(catalog.drop_table("") == true);
        REQUIRE(catalog.get_table_count() == 0);
    }

    SECTION("Case sensitivity operations") {
        // 测试表名大小写敏感性
        REQUIRE(catalog.create_table("Users", schema) == true);
        REQUIRE(catalog.table_exists("Users") == true);
        REQUIRE(catalog.table_exists("users") == false); // 应该区分大小写
        REQUIRE(catalog.table_exists("USERS") == false);
        
        REQUIRE(catalog.get_table_count() == 1);
        
        // 验证只有正确大小写的表名可以访问
        REQUIRE(catalog.get_table("Users") != nullptr);
        REQUIRE(catalog.get_table("users") == nullptr);
        REQUIRE(catalog.get_table("USERS") == nullptr);
    }

    SECTION("Special characters in table names") {
        // 测试特殊字符表名
        REQUIRE(catalog.create_table("user_profiles", schema) == true);
        REQUIRE(catalog.create_table("order-details", schema) == true);
        REQUIRE(catalog.create_table("test123", schema) == true);
        
        REQUIRE(catalog.get_table_count() == 3);
        
        // 验证特殊字符表名的访问
        REQUIRE(catalog.get_table("user_profiles") != nullptr);
        REQUIRE(catalog.get_table("order-details") != nullptr);
        REQUIRE(catalog.get_table("test123") != nullptr);
    }
}

TEST_CASE("CatalogManager stress tests", "[catalog][stress][unit]")
{
    minidb::CatalogManager catalog;
    
    std::vector<minidb::Column> columns = {
        minidb::Column{"id", minidb::TypeId::INTEGER, 4, 0}
    };
    
    minidb::Schema schema(columns);

    SECTION("Multiple table creation and deletion") {
        const int TABLE_COUNT = 100;
        
        // 创建大量表
        for (int i = 0; i < TABLE_COUNT; ++i) {
            std::string table_name = "table_" + std::to_string(i);
            REQUIRE(catalog.create_table(table_name, schema) == true);
        }
        
        REQUIRE(catalog.get_table_count() == TABLE_COUNT);
        
        // 验证所有表都存在
        for (int i = 0; i < TABLE_COUNT; ++i) {
            std::string table_name = "table_" + std::to_string(i);
            REQUIRE(catalog.table_exists(table_name) == true);
            REQUIRE(catalog.get_table(table_name) != nullptr);
        }
        
        // 删除所有表
        for (int i = 0; i < TABLE_COUNT; ++i) {
            std::string table_name = "table_" + std::to_string(i);
            REQUIRE(catalog.drop_table(table_name) == true);
        }
        
        REQUIRE(catalog.get_table_count() == 0);
        
        // 验证所有表都不存在
        for (int i = 0; i < TABLE_COUNT; ++i) {
            std::string table_name = "table_" + std::to_string(i);
            REQUIRE(catalog.table_exists(table_name) == false);
            REQUIRE(catalog.get_table(table_name) == nullptr);
        }
    }

    SECTION("Repeated operations on same table") {
        const int ITERATIONS = 50;
        
        // 重复创建和删除同一个表
        for (int i = 0; i < ITERATIONS; ++i) {
            REQUIRE(catalog.create_table("test_table", schema) == true);
            REQUIRE(catalog.table_exists("test_table") == true);
            REQUIRE(catalog.get_table_count() == 1);
            
            REQUIRE(catalog.drop_table("test_table") == true);
            REQUIRE(catalog.table_exists("test_table") == false);
            REQUIRE(catalog.get_table_count() == 0);
        }
    }
}