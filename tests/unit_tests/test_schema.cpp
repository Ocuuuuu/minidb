#include <../tests/catch2/catch_amalgamated.hpp>
#include <engine/catalog/schema.h>
#include <stdexcept>

TEST_CASE("Schema class functionality", "[schema][catalog][unit]")
{
    // 创建测试用的列定义
    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0},
        minidb::MyColumn{"name", minidb::TypeId::VARCHAR, 50, 0},
        minidb::MyColumn{"age", minidb::TypeId::INTEGER, 4, 0},
        minidb::MyColumn{"is_active", minidb::TypeId::BOOLEAN, 1, 0}
    };

    minidb::Schema schema(columns);

    SECTION("Schema construction and basic properties") {
        REQUIRE(schema.get_column_count() == 4);
        REQUIRE(schema.get_length() == 59); // 4 + 50 + 4 + 1 = 59
    }

    SECTION("Column offset calculation") {
        REQUIRE(schema.get_column(static_cast<uint32_t>(0)).offset == 0);   // id 偏移量 0
        REQUIRE(schema.get_column(static_cast<uint32_t>(1)).offset == 4);   // name 偏移量 4
        REQUIRE(schema.get_column(static_cast<uint32_t>(2)).offset == 54);  // age 偏移量 54 (4 + 50)
        REQUIRE(schema.get_column(static_cast<uint32_t>(3)).offset == 58);  // is_active 偏移量 58 (54 + 4)
    }

    SECTION("Get column by index") {
        const auto& id_col = schema.get_column(static_cast<uint32_t>(0));
        REQUIRE(id_col.name == "id");
        REQUIRE(id_col.length == 4);
        REQUIRE(id_col.type == minidb::TypeId::INTEGER);

        const auto& name_col = schema.get_column(static_cast<uint32_t>(1));
        REQUIRE(name_col.name == "name");
        REQUIRE(name_col.length == 50);
        REQUIRE(name_col.type == minidb::TypeId::VARCHAR);

        // 测试越界访问
        REQUIRE_THROWS_AS(schema.get_column(static_cast<uint32_t>(10)), std::out_of_range);
        REQUIRE_THROWS_AS(schema.get_column(static_cast<uint32_t>(100)), std::out_of_range);
    }

    SECTION("Get column index by name") {
        REQUIRE(schema.get_column_index("id") == 0);
        REQUIRE(schema.get_column_index("name") == 1);
        REQUIRE(schema.get_column_index("age") == 2);
        REQUIRE(schema.get_column_index("is_active") == 3);

        // 测试不存在的列名
        REQUIRE_THROWS_AS(schema.get_column_index("nonexistent"), std::runtime_error);
        REQUIRE_THROWS_AS(schema.get_column_index(""), std::runtime_error);

        // 测试大小写敏感性
        REQUIRE_THROWS_AS(schema.get_column_index("ID"), std::runtime_error);
        REQUIRE_THROWS_AS(schema.get_column_index("Name"), std::runtime_error);
    }

    // 新增测试：has_column 方法
    SECTION("Check column existence with has_column") {
        REQUIRE(schema.has_column("id") == true);
        REQUIRE(schema.has_column("name") == true);
        REQUIRE(schema.has_column("age") == true);
        REQUIRE(schema.has_column("is_active") == true);

        // 测试不存在的列
        REQUIRE(schema.has_column("nonexistent") == false);
        REQUIRE(schema.has_column("") == false);
        REQUIRE(schema.has_column("ID") == false); // 大小写敏感
        REQUIRE(schema.has_column("Name") == false); // 大小写敏感
    }

    // 新增测试：get_column by name 方法
    SECTION("Get column by name") {
        const auto& id_col = schema.get_column("id");
        REQUIRE(id_col.name == "id");
        REQUIRE(id_col.type == minidb::TypeId::INTEGER);
        REQUIRE(id_col.length == 4);

        const auto& name_col = schema.get_column("name");
        REQUIRE(name_col.name == "name");
        REQUIRE(name_col.type == minidb::TypeId::VARCHAR);
        REQUIRE(name_col.length == 50);

        // 测试不存在的列名
        REQUIRE_THROWS_AS(schema.get_column("nonexistent"), std::runtime_error);
        REQUIRE_THROWS_AS(schema.get_column(""), std::runtime_error);
        REQUIRE_THROWS_AS(schema.get_column("ID"), std::runtime_error); // 大小写敏感
    }

    // 新增测试：get_column by name (mutable version)
    SECTION("Get mutable column by name") {
        minidb::Schema mutable_schema(columns); // 非const对象

        auto& id_col = mutable_schema.get_column("id");
        REQUIRE(id_col.name == "id");

        // 可以修改列属性（谨慎使用）
        id_col.name = "user_id"; // 修改列名
        REQUIRE(id_col.name == "user_id");

        // 验证修改后的列
        REQUIRE(mutable_schema.has_column("user_id") == true);
        REQUIRE(mutable_schema.has_column("id") == false);

        // 测试不存在的列名
        REQUIRE_THROWS_AS(mutable_schema.get_column("nonexistent"), std::runtime_error);
    }

    // 新增测试：get_column_names 方法
    SECTION("Get all column names") {
        auto column_names = schema.get_column_names();
        REQUIRE(column_names.size() == 4);
        REQUIRE(column_names[0] == "id");
        REQUIRE(column_names[1] == "name");
        REQUIRE(column_names[2] == "age");
        REQUIRE(column_names[3] == "is_active");

        // 验证顺序与定义顺序一致
        REQUIRE(column_names == std::vector<std::string>{"id", "name", "age", "is_active"});
    }
}

TEST_CASE("Schema edge cases", "[schema][edge][unit]")
{
    SECTION("Empty columns list") {
        std::vector<minidb::MyColumn> empty_columns;
        minidb::Schema schema(empty_columns);

        REQUIRE(schema.get_column_count() == 0);
        REQUIRE(schema.get_length() == 0);

        // 测试空模式下的异常
        REQUIRE_THROWS_AS(schema.get_column(static_cast<uint32_t>(0)), std::out_of_range);
        REQUIRE_THROWS_AS(schema.get_column_index("any"), std::runtime_error);

        // 新增：测试空模式下的新方法
        REQUIRE(schema.has_column("any") == false);
        REQUIRE_THROWS_AS(schema.get_column("any"), std::runtime_error);
        REQUIRE(schema.get_column_names().empty() == true);
    }

    SECTION("Single column schema") {
        std::vector<minidb::MyColumn> single_column = {
            minidb::MyColumn{"single", minidb::TypeId::INTEGER, 4, 0}
        };

        minidb::Schema schema(single_column);

        REQUIRE(schema.get_column_count() == 1);
        REQUIRE(schema.get_length() == 4);
        REQUIRE(schema.get_column(static_cast<uint32_t>(0)).offset == 0);
        REQUIRE(schema.get_column_index("single") == 0);

        // 新增：测试单列模式下的新方法
        REQUIRE(schema.has_column("single") == true);
        REQUIRE(schema.has_column("other") == false);

        const auto& col = schema.get_column("single");
        REQUIRE(col.name == "single");

        auto names = schema.get_column_names();
        REQUIRE(names.size() == 1);
        REQUIRE(names[0] == "single");
    }
}

TEST_CASE("Schema with all TypeId types", "[schema][types][unit]")
{
    SECTION("All supported TypeId types") {
        std::vector<minidb::MyColumn> all_types_columns = {
            minidb::MyColumn{"bool_col", minidb::TypeId::BOOLEAN, 1, 0},
            minidb::MyColumn{"int_col", minidb::TypeId::INTEGER, 4, 0},
            minidb::MyColumn{"varchar_col", minidb::TypeId::VARCHAR, 100, 0}
        };

        minidb::Schema schema(all_types_columns);

        REQUIRE(schema.get_column_count() == 3);
        REQUIRE(schema.get_length() == 105); // 1 + 4 + 100 = 105

        // 验证类型正确性
        REQUIRE(schema.get_column(static_cast<uint32_t>(0)).type == minidb::TypeId::BOOLEAN);
        REQUIRE(schema.get_column(static_cast<uint32_t>(1)).type == minidb::TypeId::INTEGER);
        REQUIRE(schema.get_column(static_cast<uint32_t>(2)).type == minidb::TypeId::VARCHAR);

        // 新增：测试新方法
        REQUIRE(schema.has_column("bool_col") == true);
        REQUIRE(schema.has_column("int_col") == true);
        REQUIRE(schema.has_column("varchar_col") == true);

        auto names = schema.get_column_names();
        REQUIRE(names == std::vector<std::string>{"bool_col", "int_col", "varchar_col"});
    }
}

// 新增测试：方法之间的协调性
TEST_CASE("Schema method consistency", "[schema][consistency][unit]")
{
    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0},
        minidb::MyColumn{"name", minidb::TypeId::VARCHAR, 50, 0}
    };

    minidb::Schema schema(columns);

    SECTION("has_column and get_column consistency") {
        REQUIRE(schema.has_column("id") == true);
        REQUIRE_NOTHROW(schema.get_column("id"));

        REQUIRE(schema.has_column("nonexistent") == false);
        REQUIRE_THROWS_AS(schema.get_column("nonexistent"), std::runtime_error);
    }

    SECTION("get_column_index and get_column consistency") {
        uint32_t index = schema.get_column_index("name");
        const auto& col_by_index = schema.get_column(index); // 这里使用uint32_t，不需要转换
        const auto& col_by_name = schema.get_column("name");

        REQUIRE(col_by_index.name == col_by_name.name);
        REQUIRE(col_by_index.type == col_by_name.type);
        REQUIRE(col_by_index.length == col_by_name.length);
    }

    SECTION("get_column_names completeness") {
        auto names = schema.get_column_names();
        REQUIRE(names.size() == schema.get_column_count());

        for (const auto& name : names) {
            REQUIRE(schema.has_column(name) == true);
            REQUIRE_NOTHROW(schema.get_column(name));
        }
    }
}

TEST_CASE("Column validation tests", "[schema][validation][unit]")
{
    SECTION("Invalid column name") {
        // 空列名
        REQUIRE_THROWS_AS(minidb::MyColumn("", minidb::TypeId::INTEGER, 4, 0),
                         std::invalid_argument);

        // 包含空格的列名
        REQUIRE_THROWS_AS(minidb::MyColumn("first name", minidb::TypeId::INTEGER, 4, 0),
                         std::invalid_argument);
    }

    SECTION("Invalid column type") {
        // INVALID 类型
        REQUIRE_THROWS_AS(minidb::MyColumn("test", minidb::TypeId::INVALID, 4, 0),
                         std::invalid_argument);
    }

    SECTION("Invalid column length") {
        // 长度为0
        REQUIRE_THROWS_AS(minidb::MyColumn("test", minidb::TypeId::INTEGER, 0, 0),
                         std::invalid_argument);

        // BOOLEAN 长度不为1
        REQUIRE_THROWS_AS(minidb::MyColumn("test", minidb::TypeId::BOOLEAN, 4, 0),
                         std::invalid_argument);

        // INTEGER 长度无效
        REQUIRE_THROWS_AS(minidb::MyColumn("test", minidb::TypeId::INTEGER, 8, 0),
                         std::invalid_argument);

        // VARCHAR 长度过大
        REQUIRE_THROWS_AS(minidb::MyColumn("test", minidb::TypeId::VARCHAR, 70000, 0),
                         std::invalid_argument);
    }

    SECTION("Valid integer lengths") {
        // 测试所有有效的 INTEGER 长度
        REQUIRE_NOTHROW(minidb::MyColumn("byte", minidb::TypeId::INTEGER, 1, 0));
        REQUIRE_NOTHROW(minidb::MyColumn("short", minidb::TypeId::INTEGER, 2, 0));
        REQUIRE_NOTHROW(minidb::MyColumn("int", minidb::TypeId::INTEGER, 4, 0));
    }
}

TEST_CASE("Simple exception test", "[debug]")
{
    // 测试标准异常是否能被正确捕获
    REQUIRE_THROWS_AS(throw std::invalid_argument("test"), std::invalid_argument);

    // 测试你的具体异常
    REQUIRE_THROWS_AS(
        minidb::MyColumn("test", minidb::TypeId::INVALID, 4, 0),
        std::invalid_argument
    );
}