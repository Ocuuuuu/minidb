//语义分析器测试
// Created by tang_ on 2025/9/12.
//


// 引入 Catch2 测试框架头文件
#include <iostream>
#include "../catch2/catch_amalgamated.hpp"

#include "../../include/compiler/SemanticAnalyzer.h"
#include "../../include/compiler/AST.h"
#include "../../include/engine/catalog/catalog_manager.h"
#include "../../src/compiler/SemanticAnalyzer.cpp"


using namespace minidb;

// 测试 stringToTypeId 函数
TEST_CASE("TypeId conversion", "[semantic][type]") {
    SECTION("Valid type conversions") {
        REQUIRE(stringToTypeId("int") == TypeId::INTEGER);
        REQUIRE(stringToTypeId("INTEGER") == TypeId::INTEGER);
        REQUIRE(stringToTypeId("string") == TypeId::VARCHAR);
        REQUIRE(stringToTypeId("VARCHAR") == TypeId::VARCHAR);
        REQUIRE(stringToTypeId("float") == TypeId::FLOAT);
        REQUIRE(stringToTypeId("boolean") == TypeId::BOOLEAN);
    }

    SECTION("Invalid type conversions") {
        REQUIRE(stringToTypeId("invalid_type") == TypeId::INVALID);
        REQUIRE(stringToTypeId("") == TypeId::INVALID);
    }
}

// 测试 CREATE TABLE 语句的语义分析
TEST_CASE("CREATE TABLE analysis", "[semantic][create]") {
    CatalogManager catalog_manager;
    SemanticAnalyzer analyzer(catalog_manager);

    SECTION("Valid CREATE TABLE") {
        auto ast = std::make_unique<CreateTableAST>();
        ast->tableName = "test_table";
        ast->columns = {
            {"id", "int"},
            {"name", "varchar"},
            {"age", "integer"}
        };

        REQUIRE_NOTHROW(analyzer.analyze(ast.get()));
        REQUIRE(catalog_manager.table_exists("test_table"));
    }

    SECTION("Duplicate table name") {
        auto ast = std::make_unique<CreateTableAST>();
        ast->tableName = "test_table";
        ast->columns = {
            {"id", "int"}
        };

        // 第一次创建应该成功
        REQUIRE_NOTHROW(analyzer.analyze(ast.get()));

        // 第二次创建应该失败
        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }

    SECTION("Duplicate column names") {
        auto ast = std::make_unique<CreateTableAST>();
        ast->tableName = "test_table_dup_col";
        ast->columns = {
            {"id", "int"},
            {"id", "varchar"} // 重复列名
        };

        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }

    SECTION("Invalid column type") {
        auto ast = std::make_unique<CreateTableAST>();
        ast->tableName = "test_table_invalid_type";
        ast->columns = {
            {"id", "invalid_type"} // 非法类型
        };

        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }
}

// 测试 INSERT 语句的语义分析
TEST_CASE("INSERT analysis", "[semantic][insert]") {
    CatalogManager catalog_manager;
    SemanticAnalyzer analyzer(catalog_manager);

    // 先创建测试表
    auto create_ast = std::make_unique<CreateTableAST>();
    create_ast->tableName = "test_table";
    create_ast->columns = {
        {"id", "int"},
        {"name", "varchar"},
        {"age", "integer"}
    };
    analyzer.analyze(create_ast.get());

    SECTION("Valid INSERT") {
        auto ast = std::make_unique<InsertAST>();
        ast->tableName = "test_table";
        ast->values = {"1", "'Alice'", "25"};

        REQUIRE_NOTHROW(analyzer.analyze(ast.get()));
    }

    SECTION("Table not exists") {
        auto ast = std::make_unique<InsertAST>();
        ast->tableName = "nonexistent_table";
        ast->values = {"1"};

        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }

    SECTION("Value count mismatch") {
        auto ast = std::make_unique<InsertAST>();
        ast->tableName = "test_table";
        ast->values = {"1", "'Alice'"}; // 缺少 age 值

        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }

    SECTION("Type mismatch") {
        auto ast = std::make_unique<InsertAST>();
        ast->tableName = "test_table";
        ast->values = {"'not_a_number'", "'Alice'", "25"}; // id 应该是数字

        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }
}

// 测试 SELECT 语句的语义分析
TEST_CASE("SELECT analysis", "[semantic][select]") {
    CatalogManager catalog_manager;
    SemanticAnalyzer analyzer(catalog_manager);

    // 先创建测试表
    auto create_ast = std::make_unique<CreateTableAST>();
    create_ast->tableName = "test_table";
    create_ast->columns = {
        {"id", "int"},
        {"name", "varchar"},
        {"age", "integer"}
    };
    analyzer.analyze(create_ast.get());

    SECTION("Valid SELECT *") {
        auto ast = std::make_unique<SelectAST>();
        ast->tableName = "test_table";
        ast->columns = {"*"};

        REQUIRE_NOTHROW(analyzer.analyze(ast.get()));
    }

    SECTION("Valid SELECT specific columns") {
        auto ast = std::make_unique<SelectAST>();
        ast->tableName = "test_table";
        ast->columns = {"id", "name"};

        REQUIRE_NOTHROW(analyzer.analyze(ast.get()));
    }

    SECTION("Table not exists") {
        auto ast = std::make_unique<SelectAST>();
        ast->tableName = "nonexistent_table";
        ast->columns = {"*"};

        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }

    SECTION("Column not exists") {
        auto ast = std::make_unique<SelectAST>();
        ast->tableName = "test_table";
        ast->columns = {"nonexistent_column"};

        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }
}

// 测试 DELETE 语句的语义分析
TEST_CASE("DELETE analysis", "[semantic][delete]") {
    CatalogManager catalog_manager;
    SemanticAnalyzer analyzer(catalog_manager);

    // 先创建测试表
    auto create_ast = std::make_unique<CreateTableAST>();
    create_ast->tableName = "test_table";
    create_ast->columns = {
        {"id", "int"},
        {"name", "varchar"}
    };
    analyzer.analyze(create_ast.get());

    SECTION("Valid DELETE") {
        auto ast = std::make_unique<DeleteAST>();
        ast->tableName = "test_table";

        REQUIRE_NOTHROW(analyzer.analyze(ast.get()));
    }

    SECTION("Table not exists") {
        auto ast = std::make_unique<DeleteAST>();
        ast->tableName = "nonexistent_table";

        REQUIRE_THROWS_AS(analyzer.analyze(ast.get()), SemanticError);
    }
}
