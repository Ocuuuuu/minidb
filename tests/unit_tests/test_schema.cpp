#include <../tests/catch2/catch_amalgamated.hpp>
#include <engine/catalog/schema.h>
#include <stdexcept>

TEST_CASE("Schema class functionality", "[schema][catalog][unit]")
{
    // 创建测试用的列定义 - 使用正确的 TypeId 和有效长度
    std::vector<minidb::Column> columns = {
        minidb::Column{"id", minidb::TypeId::INTEGER, 4, 0},
        minidb::Column{"name", minidb::TypeId::VARCHAR, 50, 0},
        minidb::Column{"age", minidb::TypeId::INTEGER, 4, 0},
        minidb::Column{"is_active", minidb::TypeId::BOOLEAN, 1, 0}
    };

    minidb::Schema schema(columns);

    SECTION("Schema construction and basic properties") {
        REQUIRE(schema.get_column_count() == 4);
        REQUIRE(schema.get_length() == 59); // 4 + 50 + 4 + 1 = 59
    }

    SECTION("Column offset calculation") {
        REQUIRE(schema.get_column(0).offset == 0);   // id 偏移量 0
        REQUIRE(schema.get_column(1).offset == 4);   // name 偏移量 4
        REQUIRE(schema.get_column(2).offset == 54);  // age 偏移量 54 (4 + 50)
        REQUIRE(schema.get_column(3).offset == 58);  // is_active 偏移量 58 (54 + 4)
    }

    SECTION("Get column by index") {
        const auto& id_col = schema.get_column(0);
        REQUIRE(id_col.name == "id");
        REQUIRE(id_col.length == 4);
        REQUIRE(id_col.type == minidb::TypeId::INTEGER);

        const auto& name_col = schema.get_column(1);
        REQUIRE(name_col.name == "name");
        REQUIRE(name_col.length == 50);
        REQUIRE(name_col.type == minidb::TypeId::VARCHAR);

        // 测试越界访问
        REQUIRE_THROWS_AS(schema.get_column(10), std::out_of_range);
        REQUIRE_THROWS_AS(schema.get_column(100), std::out_of_range);
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
}

TEST_CASE("Schema edge cases", "[schema][edge][unit]")
{
    SECTION("Empty columns list") {
        std::vector<minidb::Column> empty_columns;
        minidb::Schema schema(empty_columns);

        REQUIRE(schema.get_column_count() == 0);
        REQUIRE(schema.get_length() == 0);

        // 测试空模式下的异常
        REQUIRE_THROWS_AS(schema.get_column(0), std::out_of_range);
        REQUIRE_THROWS_AS(schema.get_column_index("any"), std::runtime_error);
    }

    SECTION("Single column schema") {
        std::vector<minidb::Column> single_column = {
            minidb::Column{"single", minidb::TypeId::INTEGER, 4, 0}
        };

        minidb::Schema schema(single_column);

        REQUIRE(schema.get_column_count() == 1);
        REQUIRE(schema.get_length() == 4);
        REQUIRE(schema.get_column(0).offset == 0);
        REQUIRE(schema.get_column_index("single") == 0);
    }
}

TEST_CASE("Schema with all TypeId types", "[schema][types][unit]")
{
    SECTION("All supported TypeId types") {
        std::vector<minidb::Column> all_types_columns = {
            minidb::Column{"bool_col", minidb::TypeId::BOOLEAN, 1, 0},
            minidb::Column{"int_col", minidb::TypeId::INTEGER, 4, 0},
            minidb::Column{"varchar_col", minidb::TypeId::VARCHAR, 100, 0}
        };

        minidb::Schema schema(all_types_columns);

        REQUIRE(schema.get_column_count() == 3);
        REQUIRE(schema.get_length() == 105); // 1 + 4 + 100 = 105

        // 验证类型正确性
        REQUIRE(schema.get_column(0).type == minidb::TypeId::BOOLEAN);
        REQUIRE(schema.get_column(1).type == minidb::TypeId::INTEGER);
        REQUIRE(schema.get_column(2).type == minidb::TypeId::VARCHAR);
    }

    SECTION("Valid type combinations") {
        // 测试各种有效的类型组合
        std::vector<minidb::Column> mixed_columns = {
            minidb::Column{"id", minidb::TypeId::INTEGER, 4, 0},
            minidb::Column{"name", minidb::TypeId::VARCHAR, 50, 0},
            minidb::Column{"active", minidb::TypeId::BOOLEAN, 1, 0},
            minidb::Column{"description", minidb::TypeId::VARCHAR, 200, 0}
        };

        minidb::Schema schema(mixed_columns);
        REQUIRE(schema.get_column_count() == 4);
        REQUIRE(schema.get_length() == 255); // 4 + 50 + 1 + 200 = 255
    }
}

TEST_CASE("Column validation tests", "[schema][validation][unit]")
{
    SECTION("Invalid column name") {
        // 空列名
        REQUIRE_THROWS_AS(minidb::Column("", minidb::TypeId::INTEGER, 4, 0),
                         std::invalid_argument);

        // 包含空格的列名
        REQUIRE_THROWS_AS(minidb::Column("first name", minidb::TypeId::INTEGER, 4, 0),
                         std::invalid_argument);
    }

    SECTION("Invalid column type") {
        // INVALID 类型
        REQUIRE_THROWS_AS(minidb::Column("test", minidb::TypeId::INVALID, 4, 0),
                         std::invalid_argument);
    }

    SECTION("Invalid column length") {
        // 长度为0
        REQUIRE_THROWS_AS(minidb::Column("test", minidb::TypeId::INTEGER, 0, 0),
                         std::invalid_argument);

        // BOOLEAN 长度不为1
        REQUIRE_THROWS_AS(minidb::Column("test", minidb::TypeId::BOOLEAN, 4, 0),
                         std::invalid_argument);

        // INTEGER 长度无效
        REQUIRE_THROWS_AS(minidb::Column("test", minidb::TypeId::INTEGER, 8, 0),
                         std::invalid_argument);

        // VARCHAR 长度过大
        REQUIRE_THROWS_AS(minidb::Column("test", minidb::TypeId::VARCHAR, 70000, 0),
                         std::invalid_argument);
    }

    SECTION("Valid integer lengths") {
        // 测试所有有效的 INTEGER 长度
        REQUIRE_NOTHROW(minidb::Column("byte", minidb::TypeId::INTEGER, 1, 0));
        REQUIRE_NOTHROW(minidb::Column("short", minidb::TypeId::INTEGER, 2, 0));
        REQUIRE_NOTHROW(minidb::Column("int", minidb::TypeId::INTEGER, 4, 0));
    }
}

TEST_CASE("Schema construction with invalid columns", "[schema][error][unit]")
{
    SECTION("Schema with invalid column should throw") {
        std::vector<minidb::Column> invalid_columns = {
            minidb::Column{"valid", minidb::TypeId::INTEGER, 4, 0},
            minidb::Column{"invalid", minidb::TypeId::INVALID, 4, 0} // 这个会抛出异常
        };

        // 由于 Column 构造函数会立即验证，这个测试可能无法到达 Schema 构造函数
        // 但我们可以测试 Column 构造的异常
        REQUIRE_THROWS_AS(minidb::Column("invalid", minidb::TypeId::INVALID, 4, 0),
                         std::invalid_argument);
    }
}