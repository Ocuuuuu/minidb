#include <iostream>
#include "../include/compiler/Lexer.h"
#include "../include/common/Token.h"
#include "../include/common/Value.h"
#include "../include/common/Tuple.h"
#include "../include/compiler/Parser.h"
#include "../include/compiler/AST.h"
#include "../include/engine/catalog/catalog_manager.h"
#include "../include/compiler/SemanticAnalyzer.h"

using namespace std;
using namespace minidb;



int main() {
    // // 测试Value类
    // minidb::Value int_val(42);
    // minidb::Value str_val("Hello");
    // minidb::Value bool_val(true);
    //
    // std::cout << "Integer: " << int_val << std::endl;
    // std::cout << "String: " << str_val << std::endl;
    // std::cout << "Boolean: " << bool_val << std::endl;
    //
    // // 测试Tuple类
    // std::vector<minidb::Value> values = {int_val, str_val, bool_val};
    // minidb::Tuple tuple(values);
    //
    // std::cout << "Tuple: " << tuple << std::endl;
    // std::cout << "First value: " << tuple.getValue(0) << std::endl;



    // //测试词法分析器
    // //测试用的SQL语句
    // string sql = "SELECT name, age FROM Students WHERE age > 20;";
    // //创建词法分析器并传入SQL语句
    // Lexer lexer(sql);
    // //获取所有Token
    // vector<Token> tokens = lexer.getAllTokens();
    //
    // //遍历并打印每个Token的信息
    // for (const auto& token : tokens) {
    //     cout << "Type: ";
    //     switch (token.type) {
    //     case TokenType::KEYWORD:
    //         cout << "KEYWORD (1), ";
    //         break;
    //     case TokenType::IDENTIFIER:
    //         cout << "IDENTIFIER (2), ";
    //         break;
    //     case TokenType::CONSTANT:
    //         cout << "CONSTANT (3), ";
    //         break;
    //     case TokenType::OPERATOR:
    //         cout << "OPERATOR (4), ";
    //         break;
    //     case TokenType::DELIMITER:
    //         cout << "DELIMITER (5), ";
    //         break;
    //     case TokenType::ERROR:
    //         cout << "ERROR (6), ";
    //         break;
    //     case TokenType::EOF_TOKEN:
    //         cout << "EOF (7), ";
    //         break;
    //     }
    //     cout << "Value: " << token.value
    //          << ", Line: " << token.line
    //          << ", Column: " << token.column << endl;
    // }
    //
    //

    // //测试语法分析器
    // // 测试 SELECT 语句
    // string sql1 = "SELECT name, age FROM Students WHERE age > 20;";
    // Lexer lexer1(sql1);
    // Parser parser1(lexer1);
    // auto ast1 = parser1.parse();
    // cout << "=== Parsing SELECT statement ===" << endl;
    // printAST(ast1.get());
    // cout << endl;
    //
    // 测试 CREATE TABLE 语句
    // string sql2 = "CREATE TABLE Students(name VARCHAR, age INTEGER);";
    // Lexer lexer2(sql2);
    // Parser parser2(lexer2);
    // auto ast2 = parser2.parse();
    // cout << "=== Parsing CREATE TABLE statement ===" << endl;
    // printAST(ast2.get());
    // cout << endl;
    //
    // // 测试 INSERT 语句
    // string sql3 = "INSERT INTO Students VALUES('Alice', 20);";
    // Lexer lexer3(sql3);
    // Parser parser3(lexer3);
    // auto ast3 = parser3.parse();
    // cout << "=== Parsing INSERT statement ===" << endl;
    // printAST(ast3.get());



    //测试语义分析器
    // 创建目录管理器
    CatalogManager catalog_manager;

    // 测试CREATE TABLE
    try {
        string sql_create = "CREATE TABLE Students(name VARCHAR, age INTEGER);";
        Lexer lexer_create(sql_create);
        Parser parser_create(lexer_create);
        auto ast_create = parser_create.parse();
        cout << "=== Parsing CREATE TABLE statement ===" << endl;
        printAST(ast_create.get());
        cout << endl;

        // 执行语义分析
        SemanticAnalyzer analyzer(catalog_manager);
        analyzer.analyze(ast_create.get());
        cout << "=== Semantic Check Passed: CREATE TABLE is valid ===" << endl << endl;
    } catch (const SemanticError& e) {
        cerr << "=== Semantic Error (CREATE TABLE): " << e.what() << " ===" << endl;
        return 1;
    }

    // 测试INSERT
    try {
        string sql_insert = "INSERT INTO Students VALUES(Alice, 20);";
        Lexer lexer_insert(sql_insert);
        Parser parser_insert(lexer_insert);
        auto ast_insert = parser_insert.parse();
        cout << "=== Parsing INSERT statement ===" << endl;
        printAST(ast_insert.get());
        cout << endl;

        // 执行语义分析
        SemanticAnalyzer analyzer(catalog_manager);
        analyzer.analyze(ast_insert.get());
        cout << "=== Semantic Check Passed: INSERT is valid ===" << endl << endl;
    } catch (const SemanticError& e) {
        cerr << "=== Semantic Error (INSERT): " << e.what() << " ===" << endl;
        return 1;
    }

    // 测试SELECT
    try {
        string sql_select = "SELECT name, age FROM Students WHERE age > 20;";
        Lexer lexer_select(sql_select);
        Parser parser_select(lexer_select);
        auto ast_select = parser_select.parse();
        cout << "=== Parsing SELECT statement ===" << endl;
        printAST(ast_select.get());
        cout << endl;

        // 执行语义分析
        SemanticAnalyzer analyzer(catalog_manager);
        analyzer.analyze(ast_select.get());
        cout << "=== Semantic Check Passed: SELECT is valid ===" << endl;
    } catch (const SemanticError& e) {
        cerr << "=== Semantic Error (SELECT): " << e.what() << " ===" << endl;
        return 1;
    }

    return 0;
}