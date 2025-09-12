// tests/integration_tests/test_catalog_integration.cpp
#include <../tests/catch2/catch_amalgamated.hpp>
#include <engine/catalog/catalog_manager.h>
#include <engine/catalog/schema.h>
#include <engine/catalog/table_info.h>
#include <common/Types.h>
#include <set>
#include <algorithm>

using namespace minidb;

// 模拟存储管理器（用于测试集成点）
class MockStorageManager {
public:
    bool CreateTableFile(const std::string& table_name, const Schema& schema) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto result = created_tables_.insert(table_name);
        return result.second; // 返回是否插入成功（表名唯一）
    }

    bool DeleteTableFile(const std::string& table_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        return created_tables_.erase(table_name) > 0;
    }

    bool TableFileExists(const std::string& table_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return created_tables_.find(table_name) != created_tables_.end();
    }

    size_t GetTableFileCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return created_tables_.size();
    }

private:
    mutable std::mutex mutex_;
    std::set<std::string> created_tables_;
};

// 模拟执行引擎（用于测试Catalog的消费者）
class MockExecutionEngine {
public:
    const TableInfo* GetTableInfo(CatalogManager& catalog, const std::string& table_name) {
        return catalog.get_table(table_name);
    }

    std::vector<std::string> GetAllTableNames(CatalogManager& catalog) {
        return catalog.get_table_names();
    }

    bool TableExists(CatalogManager& catalog, const std::string& table_name) {
        return catalog.table_exists(table_name);
    }
};

// 测试辅助函数
Schema create_user_schema() {
    std::vector<MyColumn> columns = {
        MyColumn{"id", TypeId::INTEGER, 4, 0},
        MyColumn{"name", TypeId::VARCHAR, 50, 4},
        MyColumn{"email", TypeId::VARCHAR, 100, 54}
    };
    return Schema(columns);
}

Schema create_product_schema() {
    std::vector<MyColumn> columns = {
        MyColumn{"product_id", TypeId::INTEGER, 4, 0},
        MyColumn{"product_name", TypeId::VARCHAR, 100, 4},
        MyColumn{"price", TypeId::INTEGER, 4, 104}
    };
    return Schema(columns);
}

CreateTableAST create_user_ast() {
    CreateTableAST ast;
    ast.tableName = "users";
    ast.columns = {
        {"id", "INTEGER"},
        {"name", "VARCHAR(50)"},
        {"email", "VARCHAR(100)"}
    };
    return ast;
}



TEST_CASE("Catalog Manager Integration with Mock Storage", "[catalog][integration][storage]")
{
    CatalogManager catalog;
    MockStorageManager storage;
    MockExecutionEngine engine;

    // 使用辅助函数创建Schema
    Schema user_schema = create_user_schema();
    Schema product_schema = create_product_schema();

    SECTION("Integrated table creation workflow") {
        // 集成点1: 存储层创建表文件
        REQUIRE(storage.CreateTableFile("users", user_schema));

        // 集成点2: Catalog创建表元数据
        REQUIRE(catalog.create_table("users", user_schema));

        // 验证双方状态一致
        REQUIRE(catalog.table_exists("users"));
        REQUIRE(storage.TableFileExists("users"));
        REQUIRE(catalog.get_table_count() == 1);
        REQUIRE(storage.GetTableFileCount() == 1);

        // 集成点3: 执行引擎通过Catalog获取表信息
        const TableInfo* table_info = engine.GetTableInfo(catalog, "users");
        REQUIRE(table_info != nullptr);
        REQUIRE(table_info->get_table_name() == "users");

        // 验证Schema信息正确传递
        const Schema& retrieved_schema = table_info->get_schema();
        REQUIRE(retrieved_schema.get_column_count() == 3); // 修改为3个列
        REQUIRE(retrieved_schema.get_column(0).name == "id");
        REQUIRE(retrieved_schema.get_column(0).type == TypeId::INTEGER);
        REQUIRE(retrieved_schema.get_column(1).name == "name");
        REQUIRE(retrieved_schema.get_column(1).type == TypeId::VARCHAR);

        // 验证表ID是有效的正整数
        REQUIRE(table_info->get_table_id() > 0);
    }

    SECTION("Multiple table integration") {
        // 创建多个表，验证集成工作流
        REQUIRE(storage.CreateTableFile("users", user_schema));
        REQUIRE(catalog.create_table("users", user_schema));

        REQUIRE(storage.CreateTableFile("products", product_schema));
        REQUIRE(catalog.create_table("products", product_schema));

        // 验证双方状态一致
        REQUIRE(catalog.get_table_count() == 2);
        REQUIRE(storage.GetTableFileCount() == 2);
        REQUIRE(catalog.table_exists("users"));
        REQUIRE(catalog.table_exists("products"));
        REQUIRE(storage.TableFileExists("users"));
        REQUIRE(storage.TableFileExists("products"));

        // 验证执行引擎能正确访问所有表
        auto table_names = engine.GetAllTableNames(catalog);
        REQUIRE(table_names.size() == 2);
        REQUIRE(std::find(table_names.begin(), table_names.end(), "users") != table_names.end());
        REQUIRE(std::find(table_names.begin(), table_names.end(), "products") != table_names.end());

        // 验证两个表的ID不同且都是有效值
        const TableInfo* users_info = catalog.get_table("users");
        const TableInfo* products_info = catalog.get_table("products");
        REQUIRE(users_info->get_table_id() != products_info->get_table_id());
        REQUIRE(users_info->get_table_id() > 0);
        REQUIRE(products_info->get_table_id() > 0);
    }

    SECTION("Integrated table deletion workflow") {
        // 先创建表
        REQUIRE(storage.CreateTableFile("temp_table", user_schema));
        REQUIRE(catalog.create_table("temp_table", user_schema));

        // 记录表ID用于后续验证
        uint32_t temp_table_id = catalog.get_table("temp_table")->get_table_id();
        REQUIRE(temp_table_id > 0);

        // 集成点: 删除表时的协作
        REQUIRE(catalog.drop_table("temp_table"));
        REQUIRE(storage.DeleteTableFile("temp_table"));

        // 验证表已从双方删除
        REQUIRE_FALSE(catalog.table_exists("temp_table"));
        REQUIRE_FALSE(storage.TableFileExists("temp_table"));
        REQUIRE_FALSE(engine.TableExists(catalog, "temp_table"));
        REQUIRE(catalog.get_table_count() == 0);
        REQUIRE(storage.GetTableFileCount() == 0);
    }

    SECTION("Table ID generation and integration") {
        // 验证表ID生成在集成环境中工作正常
        REQUIRE(storage.CreateTableFile("table1", user_schema));
        REQUIRE(catalog.create_table("table1", user_schema));

        REQUIRE(storage.CreateTableFile("table2", product_schema));
        REQUIRE(catalog.create_table("table2", product_schema));

        const TableInfo* info1 = catalog.get_table("table1");
        const TableInfo* info2 = catalog.get_table("table2");

        REQUIRE(info1 != nullptr);
        REQUIRE(info2 != nullptr);

        // 验证表ID唯一但不检查具体值（因为静态计数器的状态不确定）
        REQUIRE(info1->get_table_id() != info2->get_table_id());

        // 只验证ID是正整数
        REQUIRE(info1->get_table_id() > 0);
        REQUIRE(info2->get_table_id() > 0);

        // 验证执行引擎也能获取正确的表ID
        const TableInfo* engine_info1 = engine.GetTableInfo(catalog, "table1");
        const TableInfo* engine_info2 = engine.GetTableInfo(catalog, "table2");
        REQUIRE(engine_info1->get_table_id() == info1->get_table_id());
        REQUIRE(engine_info2->get_table_id() == info2->get_table_id());
    }

    SECTION("AST Integration Tests") {
        CreateTableAST ast = create_user_ast();

        // 测试AST创建表
        REQUIRE(catalog.create_table_from_ast(ast));
        REQUIRE(catalog.table_exists("users"));

        // 测试INSERT验证
        InsertAST insert_ast;
        insert_ast.tableName = "users";
        insert_ast.values = {"1", "'test'", "'test@example.com'"};
        REQUIRE(catalog.validate_insert_ast(insert_ast));

        // 测试SELECT验证
        SelectAST select_ast;
        select_ast.tableName = "users";
        select_ast.columns = {"id", "name"};
        REQUIRE(catalog.validate_select_ast(select_ast));
    }
}

TEST_CASE("Schema Integration Across Modules", "[schema][integration]")
{
    CatalogManager catalog;
    MockExecutionEngine engine;

    SECTION("Schema consistency across catalog and execution engine") {
        // 创建复杂Schema
        std::vector<MyColumn> columns;
        columns.emplace_back("id", TypeId::INTEGER, 4, 0);
        columns.emplace_back("timestamp", TypeId::INTEGER, 4, 4);
        columns.emplace_back("description", TypeId::VARCHAR, 200, 12);
        columns.emplace_back("enabled", TypeId::BOOLEAN, 1, 212);

        Schema complex_schema(columns);

        // 通过Catalog创建表
        REQUIRE(catalog.create_table("config", complex_schema));

        // 执行引擎获取表信息
        const TableInfo* table_info = engine.GetTableInfo(catalog, "config");
        REQUIRE(table_info != nullptr);

        // 验证表ID有效
        REQUIRE(table_info->get_table_id() > 0);

        // 验证Schema在各个模块间保持一致
        const Schema& catalog_schema = catalog.get_table("config")->get_schema();
        const Schema& engine_schema = table_info->get_schema();

        REQUIRE(catalog_schema.get_column_count() == engine_schema.get_column_count());
        REQUIRE(catalog_schema.get_length() == engine_schema.get_length());

        // 验证每个列的属性都正确传递
        for (uint32_t i = 0; i < catalog_schema.get_column_count(); ++i) {
            const MyColumn& catalog_col = catalog_schema.get_column(i);
            const MyColumn& engine_col = engine_schema.get_column(i);

            REQUIRE(catalog_col.name == engine_col.name);
            REQUIRE(catalog_col.type == engine_col.type);
            REQUIRE(catalog_col.length == engine_col.length);
            REQUIRE(catalog_col.offset == engine_col.offset);
        }
    }

    SECTION("Column index lookup integration") {
        std::vector<MyColumn> columns;
        columns.emplace_back("user_id", TypeId::INTEGER, 4, 0);
        columns.emplace_back("user_name", TypeId::VARCHAR, 50, 4);
        columns.emplace_back("is_admin", TypeId::BOOLEAN, 1, 54);

        Schema schema(columns);
        REQUIRE(catalog.create_table("permissions", schema));

        const TableInfo* table_info = engine.GetTableInfo(catalog, "permissions");
        REQUIRE(table_info != nullptr);

        const Schema& retrieved_schema = table_info->get_schema();

        // 验证存在的列
        REQUIRE(retrieved_schema.get_column_index("user_id") == 0);
        REQUIRE(retrieved_schema.get_column_index("user_name") == 1);
        REQUIRE(retrieved_schema.get_column_index("is_admin") == 2);

        // 验证不存在的列会抛出异常
        REQUIRE_THROWS_AS(retrieved_schema.get_column_index("nonexistent"), std::runtime_error);
        REQUIRE_THROWS_WITH(retrieved_schema.get_column_index("nonexistent"), "Column not found: nonexistent");
    }
}

TEST_CASE("Error Handling Integration", "[catalog][integration][error]")
{
    CatalogManager catalog;
    MockStorageManager storage;
    MockExecutionEngine engine;

    std::vector<MyColumn> columns = {
        MyColumn{"id", TypeId::INTEGER, 4, 0}
    };
    Schema schema(columns);

    SECTION("Handling non-existent tables across modules") {
        // 验证所有模块对不存在的表有一致的处理
        REQUIRE_FALSE(catalog.table_exists("nonexistent"));
        REQUIRE_FALSE(storage.TableFileExists("nonexistent"));
        REQUIRE_FALSE(engine.TableExists(catalog, "nonexistent"));
        REQUIRE(catalog.get_table("nonexistent") == nullptr);
    }

    SECTION("Duplicate table creation handling") {
        // 存储层和Catalog都应该处理重复创建
        REQUIRE(storage.CreateTableFile("duplicate", schema));
        REQUIRE(catalog.create_table("duplicate", schema));

        // 记录第一个表的ID
        uint32_t first_table_id = catalog.get_table("duplicate")->get_table_id();
        REQUIRE(first_table_id > 0);

        // 第二次创建应该失败
        REQUIRE_FALSE(storage.CreateTableFile("duplicate", schema));
        REQUIRE_FALSE(catalog.create_table("duplicate", schema));

        // 状态应该保持不变
        REQUIRE(catalog.get_table_count() == 1);
        REQUIRE(storage.GetTableFileCount() == 1);

        // 验证表ID没有改变
        REQUIRE(catalog.get_table("duplicate")->get_table_id() == first_table_id);
    }
}