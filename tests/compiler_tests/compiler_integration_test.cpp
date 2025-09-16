//
// Created by tang_ on 2025/9/16.
//�������ĸ��������ϲ��ԣ��ʷ��������﷨���������������ִ�мƻ����ɣ�

#include <iostream>
#include "../catch2/catch_amalgamated.hpp"
#include "../../include/compiler/QueryPlanner.h"
#include "../../include/compiler/Lexer.h"
#include "../../include/compiler/Parser.h"
#include "../../include/compiler/SemanticAnalyzer.h"
#include "../../include/common/Token.h"
#include "compiler/SQLCompiler.h"

using namespace minidb;
using json = nlohmann::json;

// ��TokenTypeת��Ϊ�ַ��������ڵ������
static std::string tokenTypeToString(TokenType type) {
    switch (type) {
    case TokenType::KEYWORD: return "KEYWORD";
    case TokenType::IDENTIFIER: return "IDENTIFIER";
    case TokenType::CONSTANT: return "CONSTANT";
    case TokenType::OPERATOR: return "OPERATOR";
    case TokenType::DELIMITER: return "DELIMITER";
    case TokenType::ERROR: return "ERROR";
    case TokenType::EOF_TOKEN: return "EOF";
    default: return "UNKNOWN";
    }
}

static std::string tokensToString(const std::vector<Token>& tokens) {
    std::string result;
    for (const auto& token : tokens) {
        result += "[" + tokenTypeToString(token.type) + ":" + token.value + "]";
    }
    return result;
}

TEST_CASE("SQLCompiler Integration Test", "[compiler]") {
    CatalogManager catalog;
    SQLCompiler compiler(catalog);

    SECTION("CREATE TABLE") {
        json plan = compiler.compile("CREATE TABLE users (id INT, name VARCHAR);");  // ��Ϊ json ����
        REQUIRE(plan["astType"] == "CreateTable");
        REQUIRE(catalog.table_exists("users")); // ��֤���Ѵ���
    }

    SECTION("INSERT") {
        compiler.compile("CREATE TABLE users (id INT, name VARCHAR);"); // �ȴ�����
        json plan = compiler.compile("INSERT INTO users VALUES (1, 'Alice');");  // ��Ϊ json ����
        REQUIRE(plan["astType"] == "Insert");
    }

    SECTION("Invalid INSERT (Table Not Exist)") {
        REQUIRE_THROWS_AS(
            compiler.compile("INSERT INTO nonexistent VALUES (1);"),
            std::runtime_error
        );
    }
}

// // ���� SQL ����������������
// TEST_CASE("Compiler Integration Test", "[compiler][integration]") {
//     // ���� CatalogManager ����
//     CatalogManager catalogManager;
//
//     SECTION("Test CREATE TABLE statement") {
//         std::string sql = "CREATE TABLE users (id INT, name VARCHAR, age INT);";
//
//         // 1. �ʷ�����
         // Lexer lexer(sql);
         // Parser parser(lexer);
//
//         auto tokens = lexer.getAllTokens();
//         REQUIRE(!tokens.empty()); // ȷ�� Token ��Ϊ��
//
//         // 2. �﷨����
         // auto ast = parser.parse();
         // REQUIRE(ast != nullptr);
         // auto createAst = dynamic_cast<CreateTableAST*>(ast.get());
         // REQUIRE(createAst != nullptr);
//         REQUIRE(createAst->tableName == "users");
//         REQUIRE(createAst->columns.size() == 3);
//         REQUIRE(createAst->columns[0].name == "id");
//
//
//         // 3. �������
//         SemanticAnalyzer analyzer(catalogManager); // ���� CatalogManager ����
//         analyzer.analyze(ast.get());
//
//         // 4. ִ�мƻ�����
//         QueryPlanner planner;
//         auto plan = planner.generatePlan(ast.get());
//         REQUIRE(plan["type"] == "CreateTable");
//         REQUIRE(plan["tableName"] == "users");
//         REQUIRE(plan["columns"].size() == 3);
//
//     }
//
//     SECTION("Test INSERT statement") {
//         std::string sql = "INSERT INTO users VALUES (1, 'Alice', 25);";
//
//         // 1. �ʷ�����
//         Lexer lexer(sql);
//         Parser parser(lexer);
//
//         auto tokens = lexer.getAllTokens();
//         REQUIRE(!tokens.empty()); // ȷ�� Token ��Ϊ��
//
//         // 2. �﷨����
//         auto ast = parser.parse();
//         REQUIRE(ast != nullptr);
//         auto insertAst = dynamic_cast<InsertAST*>(ast.get());
//         REQUIRE(insertAst != nullptr);
//
//         // 3. �������
//         SemanticAnalyzer analyzer(catalogManager); // ���� CatalogManager ����
//         analyzer.analyze(ast.get());
//
//         // 4. ִ�мƻ�����
//         QueryPlanner planner;
//         auto plan = planner.generatePlan(ast.get());
//         REQUIRE(plan["type"] == "Insert");
//         REQUIRE(plan["tableName"] == "users");
//         REQUIRE(plan["values"].size() == 3);
//     }
//
//     SECTION("Test SELECT statement") {
//         std::string sql = "SELECT name, age FROM users WHERE age > 20;";
//         // 1. �ʷ�����
//         Lexer lexer(sql);
//         Parser parser(lexer);
//
//         auto tokens = lexer.getAllTokens();
//         REQUIRE(!tokens.empty()); // ȷ�� Token ��Ϊ��
//
//         // 2. �﷨����
//         auto ast = parser.parse();
//         REQUIRE(ast != nullptr);
//         auto selectAst = dynamic_cast<SelectAST*>(ast.get());
//         REQUIRE(selectAst != nullptr);
//
//         // 3. �������
//         SemanticAnalyzer analyzer(catalogManager); // ���� CatalogManager ����
//         analyzer.analyze(ast.get());
//
//         // 4. ִ�мƻ�����
//         QueryPlanner planner;
//         auto plan = planner.generatePlan(ast.get());
//         REQUIRE(plan["type"] == "Project");
//         REQUIRE(plan["columns"].size() == 2);
//     }
//
//     SECTION("Test DELETE statement") {
//         std::string sql = "DELETE FROM users WHERE age < 18;";
//
//         // 1. �ʷ�����
//         Lexer lexer(sql);
//         Parser parser(lexer);
//
//         auto tokens = lexer.getAllTokens();
//         REQUIRE(!tokens.empty()); // ȷ�� Token ��Ϊ��
//
//         // 2. �﷨����
//         auto ast = parser.parse();
//         REQUIRE(ast != nullptr);
//         auto deleteAst = dynamic_cast<DeleteAST*>(ast.get());
//         REQUIRE(deleteAst != nullptr);
//
//         // 3. �������
//         SemanticAnalyzer analyzer(catalogManager); // ���� CatalogManager ����
//         analyzer.analyze(ast.get());
//
//         // 4. ִ�мƻ�����
//         QueryPlanner planner;
//         auto plan = planner.generatePlan(ast.get());
//         REQUIRE(plan["type"] == "Delete");
//         REQUIRE(plan["tableName"] == "users");
//     }
// }