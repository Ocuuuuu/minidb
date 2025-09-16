#include <../tests/catch2/catch_amalgamated.hpp>
#include <engine/catalog/catalog_manager.h>
#include <engine/catalog/schema.h>
#include "compiler/AST.h"
#include <stdexcept>

TEST_CASE("CatalogManager basic functionality", "[catalog][manager][unit]")
{
    minidb::CatalogManager catalog;

    // 创建测试用的列定义
    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0},
        minidb::MyColumn{"name", minidb::TypeId::VARCHAR, 50, 0}
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

    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0},
        minidb::MyColumn{"name", minidb::TypeId::VARCHAR, 50, 0}
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

    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0}
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

    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0}
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

    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0}
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

// ==================== 新增AST相关测试用例 ====================

TEST_CASE("CatalogManager AST integration", "[catalog][ast][unit]")
{
    minidb::CatalogManager catalog;

    SECTION("Create table from AST - basic types") {
        CreateTableAST create_ast;
        create_ast.tableName = "users";

        // 添加列定义
        create_ast.columns.push_back({"id", "INT"});
        create_ast.columns.push_back({"name", "VARCHAR(100)"});
        create_ast.columns.push_back({"is_active", "BOOLEAN"});

        // 测试创建表
        REQUIRE(catalog.create_table_from_ast(create_ast) == true);
        REQUIRE(catalog.table_exists("users") == true);

        // 验证表结构
        const minidb::TableInfo* table = catalog.get_table("users");
        REQUIRE(table != nullptr);
        REQUIRE(table->get_schema().get_column_count() == 3);
        REQUIRE(table->get_schema().has_column("id"));
        REQUIRE(table->get_schema().has_column("name"));
        REQUIRE(table->get_schema().has_column("is_active"));
    }

    SECTION("Create table from AST - duplicate table") {
        CreateTableAST create_ast;
        create_ast.tableName = "users";
        create_ast.columns.push_back({"id", "INT"});

        // 第一次创建应该成功
        REQUIRE(catalog.create_table_from_ast(create_ast) == true);

        // 第二次创建应该失败
        REQUIRE(catalog.create_table_from_ast(create_ast) == false);
    }

    SECTION("Create table from AST - invalid type") {
        CreateTableAST create_ast;
        create_ast.tableName = "users";
        create_ast.columns.push_back({"invalid_col", "INVALID_TYPE"});

        // 应该创建失败
        REQUIRE(catalog.create_table_from_ast(create_ast) == false);
        REQUIRE(catalog.table_exists("users") == false);
    }

    SECTION("Create table from AST - varchar with length") {
        CreateTableAST create_ast;
        create_ast.tableName = "users";
        create_ast.columns.push_back({"name", "VARCHAR(50)"});
        create_ast.columns.push_back({"description", "STRING(200)"});

        REQUIRE(catalog.create_table_from_ast(create_ast) == true);

        const minidb::TableInfo* table = catalog.get_table("users");
        REQUIRE(table != nullptr);

        // 验证VARCHAR长度
        const auto& schema = table->get_schema();
        // 这里需要根据你的Schema实现来验证长度
    }
}

TEST_CASE("CatalogManager AST validation", "[catalog][ast][validation][unit]")
{
    minidb::CatalogManager catalog;

    // 先创建一个测试表
    CreateTableAST create_ast;
    create_ast.tableName = "users";
    create_ast.columns.push_back({"id", "INT"});
    create_ast.columns.push_back({"name", "VARCHAR(50)"});
    create_ast.columns.push_back({"age", "INT"});
    REQUIRE(catalog.create_table_from_ast(create_ast) == true);

    SECTION("Validate INSERT AST - valid") {
        InsertAST insert_ast;
        insert_ast.tableName = "users";
        insert_ast.values = {"1", "'John'", "25"};

        REQUIRE(catalog.validate_insert_ast(insert_ast) == true);
    }

    SECTION("Validate INSERT AST - table not exists") {
        InsertAST insert_ast;
        insert_ast.tableName = "nonexistent";
        insert_ast.values = {"1", "'John'"};

        REQUIRE(catalog.validate_insert_ast(insert_ast) == false);
    }

    SECTION("Validate INSERT AST - wrong value count") {
        InsertAST insert_ast;
        insert_ast.tableName = "users";
        insert_ast.values = {"1", "'John'"}; // 缺少一个值

        REQUIRE(catalog.validate_insert_ast(insert_ast) == false);

        insert_ast.values = {"1", "'John'", "25", "extra"}; // 多一个值
        REQUIRE(catalog.validate_insert_ast(insert_ast) == false);
    }

    SECTION("Validate SELECT AST - valid with specific columns") {
        SelectAST select_ast;
        select_ast.tableName = "users";
        select_ast.columns = {"id", "name"};

        REQUIRE(catalog.validate_select_ast(select_ast) == true);
    }

    SECTION("Validate SELECT AST - valid with wildcard") {
        SelectAST select_ast;
        select_ast.tableName = "users";
        select_ast.columns = {"*"};

        REQUIRE(catalog.validate_select_ast(select_ast) == true);
    }

    SECTION("Validate SELECT AST - valid with condition") {
        SelectAST select_ast;
        select_ast.tableName = "users";
        select_ast.columns = {"id", "name"};
        select_ast.condition = Condition{"id", "=", "1"};

        REQUIRE(catalog.validate_select_ast(select_ast) == true);
    }

    SECTION("Validate SELECT AST - invalid column") {
        SelectAST select_ast;
        select_ast.tableName = "users";
        select_ast.columns = {"invalid_column"};

        REQUIRE(catalog.validate_select_ast(select_ast) == false);
    }

    SECTION("Validate SELECT AST - invalid condition column") {
        SelectAST select_ast;
        select_ast.tableName = "users";
        select_ast.columns = {"id"};
        select_ast.condition = Condition{"invalid_column", "=", "1"};

        REQUIRE(catalog.validate_select_ast(select_ast) == false);
    }

    SECTION("Validate SELECT AST - table not exists") {
        SelectAST select_ast;
        select_ast.tableName = "nonexistent";
        select_ast.columns = {"id"};

        REQUIRE(catalog.validate_select_ast(select_ast) == false);
    }

    SECTION("Validate SELECT AST - mixed valid and invalid columns") {
        SelectAST select_ast;
        select_ast.tableName = "users";
        select_ast.columns = {"id", "invalid_column"};

        REQUIRE(catalog.validate_select_ast(select_ast) == false);
    }
}

TEST_CASE("CatalogManager type conversion", "[catalog][type][conversion][unit]")
{
    minidb::CatalogManager catalog;

    SECTION("Type conversion through table creation") {
        CreateTableAST create_ast;
        create_ast.tableName = "test_table";
        create_ast.columns.push_back({"id", "INT"});
        create_ast.columns.push_back({"name", "VARCHAR(100)"});
        create_ast.columns.push_back({"is_active", "BOOLEAN"});

        // 通过创建表来间接测试类型转换
        REQUIRE(catalog.create_table_from_ast(create_ast) == true);

        const minidb::TableInfo* table = catalog.get_table("test_table");
        REQUIRE(table != nullptr);

        // 验证表结构来确认类型转换正确
        const auto& schema = table->get_schema();
        // 这里可以根据你的Schema实现来验证列类型
    }
}

TEST_CASE("CatalogManager varchar length calculation", "[catalog][varchar][length][unit]")
{
    minidb::CatalogManager catalog;

    SECTION("VARCHAR length through table creation") {
        CreateTableAST create_ast;
        create_ast.tableName = "test_table";
        create_ast.columns.push_back({"short_name", "VARCHAR(50)"});
        create_ast.columns.push_back({"long_name", "VARCHAR(255)"});
        create_ast.columns.push_back({"default_name", "VARCHAR"}); // 应该使用默认长度

        REQUIRE(catalog.create_table_from_ast(create_ast) == true);

        const minidb::TableInfo* table = catalog.get_table("test_table");
        REQUIRE(table != nullptr);

        // 这里可以根据你的Schema实现来验证VARCHAR长度
        // 例如，如果Schema提供了获取列长度的方法：
        // REQUIRE(table->get_schema().get_column("short_name").get_length() == 50);
        // REQUIRE(table->get_schema().get_column("long_name").get_length() == 255);
        // REQUIRE(table->get_schema().get_column("default_name").get_length() == 255);
    }
}

TEST_CASE("CatalogManager get table schema", "[catalog][schema][unit]")
{
    minidb::CatalogManager catalog;

    // 先创建一个测试表
    CreateTableAST create_ast;
    create_ast.tableName = "users";
    create_ast.columns.push_back({"id", "INT"});
    create_ast.columns.push_back({"name", "VARCHAR(50)"});
    REQUIRE(catalog.create_table_from_ast(create_ast) == true);

    SECTION("Get table schema - exists") {
        auto schema = catalog.get_table_schema("users");
        REQUIRE(schema != nullptr);
        REQUIRE(schema->get_column_count() == 2);
        REQUIRE(schema->has_column("id"));
        REQUIRE(schema->has_column("name"));
    }

    SECTION("Get table schema - not exists") {
        auto schema = catalog.get_table_schema("nonexistent");
        REQUIRE(schema == nullptr);
    }

    SECTION("Get table schema - empty name") {
        auto schema = catalog.get_table_schema("");
        REQUIRE(schema == nullptr);
    }

    SECTION("Get table schema - multiple tables") {
        // 创建第二个表
        CreateTableAST create_ast2;
        create_ast2.tableName = "products";
        create_ast2.columns.push_back({"product_id", "INT"});
        REQUIRE(catalog.create_table_from_ast(create_ast2) == true);

        // 验证两个表的schema都能正确获取
        auto users_schema = catalog.get_table_schema("users");
        auto products_schema = catalog.get_table_schema("products");

        REQUIRE(users_schema != nullptr);
        REQUIRE(products_schema != nullptr);
        REQUIRE(users_schema->get_column_count() == 2);
        REQUIRE(products_schema->get_column_count() == 1);
    }
}

TEST_CASE("CatalogManager AST edge cases", "[catalog][ast][edge][unit]")
{
    minidb::CatalogManager catalog;

    SECTION("Empty table name in AST") {
        CreateTableAST create_ast;
        create_ast.tableName = "";
        create_ast.columns.push_back({"id", "INT"});

        REQUIRE(catalog.create_table_from_ast(create_ast) == true);
        REQUIRE(catalog.table_exists("") == true);
    }

    SECTION("Empty column list in AST") {
        CreateTableAST create_ast;
        create_ast.tableName = "empty_table";
        // 没有列

        REQUIRE(catalog.create_table_from_ast(create_ast) == true);
        const minidb::TableInfo* table = catalog.get_table("empty_table");
        REQUIRE(table != nullptr);
        REQUIRE(table->get_schema().get_column_count() == 0);
    }

    SECTION("Duplicate column names in AST") {
        CreateTableAST create_ast;
        create_ast.tableName = "duplicate_columns";
        create_ast.columns.push_back({"id", "INT"});
        create_ast.columns.push_back({"id", "VARCHAR"}); // 重复的列名

        // 这个测试取决于你的实现是否允许重复列名
        // 如果允许，应该成功；如果不允许，应该失败
        bool result = catalog.create_table_from_ast(create_ast);
        // 这里不直接断言，因为行为取决于具体实现
    }
}