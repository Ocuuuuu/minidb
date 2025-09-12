#include <../tests/catch2/catch_amalgamated.hpp>
#include <engine/catalog/table_info.h>
#include <engine/catalog/schema.h>
#include <engine/catalog/MyColumn.h>
#include <stdexcept>

TEST_CASE("TableInfo class functionality", "[tableinfo][catalog][unit]") 
{
    // 创建测试用的列定义
    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0},
        minidb::MyColumn{"name", minidb::TypeId::VARCHAR, 50, 0},
        minidb::MyColumn{"age", minidb::TypeId::INTEGER, 4, 0}
    };

    minidb::Schema schema(columns);

    SECTION("TableInfo construction and basic properties") {
        minidb::TableInfo table("users", schema);
        
        REQUIRE(table.get_table_name() == "users");
        REQUIRE(table.get_table_id() > 0); // ID应该大于0
        REQUIRE(table.get_schema().get_column_count() == 3);
    }

    SECTION("TableInfo with specified table ID") {
        minidb::TableInfo table("products", schema, 12345);
        
        REQUIRE(table.get_table_name() == "products");
        REQUIRE(table.get_table_id() == 12345);
        REQUIRE(table.get_schema().get_column_count() == 3);
    }

    SECTION("TableInfo equality comparison") {
        minidb::TableInfo table1("users", schema, 100);
        minidb::TableInfo table2("users", schema, 100);
        minidb::TableInfo table3("products", schema, 100);
        minidb::TableInfo table4("users", schema, 200);
        
        REQUIRE(table1 == table2);
        REQUIRE_FALSE(table1 == table3);
        REQUIRE_FALSE(table1 == table4);
    }

    SECTION("TableInfo inequality comparison") {
        minidb::TableInfo table1("users", schema, 100);
        minidb::TableInfo table2("products", schema, 200);
        
        REQUIRE(table1 != table2);
        REQUIRE_FALSE(table1 != table1);
    }

    SECTION("TableInfo set_table_id method") {
        minidb::TableInfo table("temp_table", schema);
        uint32_t original_id = table.get_table_id();
        
        table.set_table_id(9999);
        REQUIRE(table.get_table_id() == 9999);
        
        // 恢复原ID
        table.set_table_id(original_id);
        REQUIRE(table.get_table_id() == original_id);
    }
}

TEST_CASE("TableInfo unique ID generation", "[tableinfo][id][unit]")
{
    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0}
    };
    minidb::Schema schema(columns);

    SECTION("Unique table IDs for different tables") {
        minidb::TableInfo table1("table1", schema);
        minidb::TableInfo table2("table2", schema);
        minidb::TableInfo table3("table3", schema);
        
        REQUIRE(table1.get_table_id() != table2.get_table_id());
        REQUIRE(table1.get_table_id() != table3.get_table_id());
        REQUIRE(table2.get_table_id() != table3.get_table_id());
    }

    SECTION("Different IDs for same table name") {
        minidb::TableInfo table1("same_name", schema);
        minidb::TableInfo table2("same_name", schema);
        
        REQUIRE(table1.get_table_name() == table2.get_table_name());
        REQUIRE(table1.get_table_id() != table2.get_table_id());
    }
}

TEST_CASE("TableInfo edge cases", "[tableinfo][edge][unit]")
{
    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0}
    };
    minidb::Schema schema(columns);

    SECTION("Empty table name") {
        minidb::TableInfo table("", schema);
        
        REQUIRE(table.get_table_name().empty());
        REQUIRE(table.get_table_id() > 0);
    }

    SECTION("Table with empty schema") {
        std::vector<minidb::MyColumn> empty_columns;
        minidb::Schema empty_schema(empty_columns);
        
        minidb::TableInfo table("empty_table", empty_schema);
        
        REQUIRE(table.get_table_name() == "empty_table");
        REQUIRE(table.get_table_id() > 0);
        REQUIRE(table.get_schema().get_column_count() == 0);
        REQUIRE(table.get_schema().get_length() == 0);
    }

    SECTION("Table with single column schema") {
        std::vector<minidb::MyColumn> single_column = {
            minidb::MyColumn{"single", minidb::TypeId::INTEGER, 4, 0}
        };
        minidb::Schema single_schema(single_column);
        
        minidb::TableInfo table("single_col_table", single_schema);
        
        REQUIRE(table.get_table_name() == "single_col_table");
        REQUIRE(table.get_schema().get_column_count() == 1);
        REQUIRE(table.get_schema().get_length() == 4);
    }
}

TEST_CASE("TableInfo schema integrity", "[tableinfo][schema][unit]")
{
    SECTION("Schema content preservation") {
        std::vector<minidb::MyColumn> columns = {
            minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0},
            minidb::MyColumn{"name", minidb::TypeId::VARCHAR, 50, 0},
            minidb::MyColumn{"age", minidb::TypeId::INTEGER, 4, 0}
        };
        minidb::Schema original_schema(columns);
        
        minidb::TableInfo table("test_table", original_schema);
        const minidb::Schema& retrieved_schema = table.get_schema();
        
        // 验证schema内容完整保留
        REQUIRE(retrieved_schema.get_column_count() == 3);
        REQUIRE(retrieved_schema.get_length() == 58); // 4 + 50 + 4
        
        REQUIRE(retrieved_schema.get_column(0).name == "id");
        REQUIRE(retrieved_schema.get_column(0).type == minidb::TypeId::INTEGER);
        REQUIRE(retrieved_schema.get_column(0).length == 4);
        
        REQUIRE(retrieved_schema.get_column(1).name == "name");
        REQUIRE(retrieved_schema.get_column(1).type == minidb::TypeId::VARCHAR);
        REQUIRE(retrieved_schema.get_column(1).length == 50);
        
        REQUIRE(retrieved_schema.get_column(2).name == "age");
        REQUIRE(retrieved_schema.get_column(2).type == minidb::TypeId::INTEGER);
        REQUIRE(retrieved_schema.get_column(2).length == 4);
    }

    SECTION("Schema offset calculation verification") {
        std::vector<minidb::MyColumn> columns = {
            minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0},
            minidb::MyColumn{"name", minidb::TypeId::VARCHAR, 50, 0},
            minidb::MyColumn{"age", minidb::TypeId::INTEGER, 4, 0}
        };
        minidb::Schema schema(columns);
        
        minidb::TableInfo table("offset_test", schema);
        const minidb::Schema& table_schema = table.get_schema();
        
        // 验证偏移量计算正确
        REQUIRE(table_schema.get_column(0).offset == 0);
        REQUIRE(table_schema.get_column(1).offset == 4);
        REQUIRE(table_schema.get_column(2).offset == 54); // 4 + 50
    }
}

TEST_CASE("TableInfo ID generation consistency", "[tableinfo][id][consistency][unit]")
{
    std::vector<minidb::MyColumn> columns = {
        minidb::MyColumn{"id", minidb::TypeId::INTEGER, 4, 0}
    };
    minidb::Schema schema(columns);

    SECTION("Sequential ID generation") {
        minidb::TableInfo table1("table1", schema);
        minidb::TableInfo table2("table2", schema);
        minidb::TableInfo table3("table3", schema);
        
        // ID应该是连续递增的（由于使用原子计数器）
        REQUIRE(table2.get_table_id() > table1.get_table_id());
        REQUIRE(table3.get_table_id() > table2.get_table_id());
    }

    SECTION("Mixed automatic and manual ID assignment") {
        minidb::TableInfo auto_table("auto_table", schema);
        minidb::TableInfo manual_table("manual_table", schema, 9999);
        minidb::TableInfo another_auto_table("another_auto", schema);
        
        REQUIRE(manual_table.get_table_id() == 9999);
        REQUIRE(auto_table.get_table_id() != 9999);
        REQUIRE(another_auto_table.get_table_id() != 9999);
        REQUIRE(another_auto_table.get_table_id() > auto_table.get_table_id());
    }
}