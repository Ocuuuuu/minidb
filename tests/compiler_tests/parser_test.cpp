// // 语法分析器测试文件
// // 使用Catch2测试框架测试SQL语法解析功能
//
// #include <iostream>
// #include <memory>
// #include <string>
//
// #include "../catch2/catch_amalgamated.hpp"
//
// #include "../../include/compiler/Lexer.h"
// #include "../../include/compiler/Parser.h"
// #include "../../include/compiler/AST.h"
//
// // 测试辅助函数：执行解析并返回AST节点指针
// std::unique_ptr<ASTNode> parseSQL(const std::string& sql) {
//     Lexer lexer(sql);
//     Parser parser(lexer);
//     return parser.parse();
// }
//
// // 测试CREATE TABLE语句解析
// TEST_CASE("CREATE TABLE statement parsing", "[parser][create]") {
//     SECTION("Simple CREATE TABLE with single column") {
//         std::string sql = "CREATE TABLE users (id INT);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto createAst = dynamic_cast<CreateTableAST*>(ast.get());
//         REQUIRE(createAst != nullptr);
//         REQUIRE(createAst->tableName == "users");
//         REQUIRE(createAst->columns.size() == 1);
//         REQUIRE(createAst->columns[0].name == "id");
//         REQUIRE(createAst->columns[0].type == "INT");
//     }
//
//     SECTION("CREATE TABLE with multiple columns") {
//         std::string sql = "CREATE TABLE students (name STRING, age INT, score FLOAT);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto createAst = dynamic_cast<CreateTableAST*>(ast.get());
//         REQUIRE(createAst != nullptr);
//         REQUIRE(createAst->tableName == "students");
//         REQUIRE(createAst->columns.size() == 3);
//         REQUIRE(createAst->columns[0].name == "name");
//         REQUIRE(createAst->columns[0].type == "STRING");
//         REQUIRE(createAst->columns[1].name == "age");
//         REQUIRE(createAst->columns[1].type == "INT");
//         REQUIRE(createAst->columns[2].name == "score");
//         REQUIRE(createAst->columns[2].type == "FLOAT");
//     }
//
//     SECTION("CREATE TABLE with case-insensitive keywords") {
//         std::string sql = "create table products (product_id int, description string);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto createAst = dynamic_cast<CreateTableAST*>(ast.get());
//         REQUIRE(createAst != nullptr);
//         REQUIRE(createAst->tableName == "products");
//         REQUIRE(createAst->columns.size() == 2);
//     }
// }
//
// // 测试INSERT语句解析
// TEST_CASE("INSERT statement parsing", "[parser][insert]") {
//     SECTION("Simple INSERT with single value") {
//         std::string sql = "INSERT INTO users VALUES (123);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto insertAst = dynamic_cast<InsertAST*>(ast.get());
//         REQUIRE(insertAst != nullptr);
//         REQUIRE(insertAst->tableName == "users");
//         REQUIRE(insertAst->values.size() == 1);
//         REQUIRE(insertAst->values[0] == "123");
//     }
//
//     SECTION("INSERT with multiple values and string constants") {
//         std::string sql = "INSERT INTO students VALUES ('Alice', 20, 95.5);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto insertAst = dynamic_cast<InsertAST*>(ast.get());
//         REQUIRE(insertAst != nullptr);
//         REQUIRE(insertAst->tableName == "students");
//         REQUIRE(insertAst->values.size() == 3);
//         REQUIRE(insertAst->values[0] == "'Alice'");
//         REQUIRE(insertAst->values[1] == "20");
//         REQUIRE(insertAst->values[2] == "95.5");
//     }
//
//     SECTION("INSERT with double quotes") {
//         std::string sql = "INSERT INTO courses VALUES (\"Math\", 3);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto insertAst = dynamic_cast<InsertAST*>(ast.get());
//         REQUIRE(insertAst != nullptr);
//         REQUIRE(insertAst->tableName == "courses");
//         REQUIRE(insertAst->values.size() == 2);
//         REQUIRE(insertAst->values[0] == "\'Math\'");
//     }
// }
//
// // 测试SELECT语句解析
// TEST_CASE("SELECT statement parsing", "[parser][select]") {
//     SECTION("Simple SELECT with all columns") {
//         std::string sql = "SELECT * FROM users;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto selectAst = dynamic_cast<SelectAST*>(ast.get());
//         REQUIRE(selectAst != nullptr);
//         REQUIRE(selectAst->tableName == "users");
//         REQUIRE(selectAst->columns.size() == 1);
//         REQUIRE(selectAst->columns[0] == "*");
//         REQUIRE_FALSE(selectAst->condition.has_value());
//     }
//
//     SECTION("SELECT with specific columns") {
//         std::string sql = "SELECT name, age FROM students;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto selectAst = dynamic_cast<SelectAST*>(ast.get());
//         REQUIRE(selectAst != nullptr);
//         REQUIRE(selectAst->tableName == "students");
//         REQUIRE(selectAst->columns.size() == 2);
//         REQUIRE(selectAst->columns[0] == "name");
//         REQUIRE(selectAst->columns[1] == "age");
//     }
//
//     SECTION("SELECT with WHERE clause") {
//         std::string sql = "SELECT * FROM students WHERE age > 20;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto selectAst = dynamic_cast<SelectAST*>(ast.get());
//         REQUIRE(selectAst != nullptr);
//         REQUIRE(selectAst->tableName == "students");
//         REQUIRE(selectAst->columns[0] == "*");
//         REQUIRE(selectAst->condition.has_value());
//         REQUIRE(selectAst->condition.value().column == "age");
//         REQUIRE(selectAst->condition.value().op == ">");
//         REQUIRE(selectAst->condition.value().value == "20");
//     }
//
//     SECTION("SELECT with complex WHERE condition") {
//         std::string sql = "SELECT name, score FROM students WHERE score >= 90;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto selectAst = dynamic_cast<SelectAST*>(ast.get());
//         REQUIRE(selectAst != nullptr);
//         REQUIRE(selectAst->tableName == "students");
//         REQUIRE(selectAst->columns.size() == 2);
//         REQUIRE(selectAst->columns[0] == "name");
//         REQUIRE(selectAst->columns[1] == "score");
//         REQUIRE(selectAst->condition.has_value());
//         REQUIRE(selectAst->condition.value().column == "score");
//         REQUIRE(selectAst->condition.value().op == ">=");
//         REQUIRE(selectAst->condition.value().value == "90");
//     }
// }
//
// // 测试DELETE语句解析
// TEST_CASE("DELETE statement parsing", "[parser][delete]") {
//     SECTION("Simple DELETE without WHERE clause") {
//         std::string sql = "DELETE FROM users;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto deleteAst = dynamic_cast<DeleteAST*>(ast.get());
//         REQUIRE(deleteAst != nullptr);
//         REQUIRE(deleteAst->tableName == "users");
//         REQUIRE_FALSE(deleteAst->condition.has_value());
//     }
//
//     SECTION("DELETE with WHERE clause") {
//         std::string sql = "DELETE FROM students WHERE age < 18;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto deleteAst = dynamic_cast<DeleteAST*>(ast.get());
//         REQUIRE(deleteAst != nullptr);
//         REQUIRE(deleteAst->tableName == "students");
//         REQUIRE(deleteAst->condition.has_value());
//         REQUIRE(deleteAst->condition.value().column == "age");
//         REQUIRE(deleteAst->condition.value().op == "<");
//         REQUIRE(deleteAst->condition.value().value == "18");
//     }
//
//     SECTION("DELETE with equality condition") {
//         std::string sql = "DELETE FROM products WHERE product_id = 5;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto deleteAst = dynamic_cast<DeleteAST*>(ast.get());
//         REQUIRE(deleteAst != nullptr);
//         REQUIRE(deleteAst->tableName == "products");
//         REQUIRE(deleteAst->condition.has_value());
//         REQUIRE(deleteAst->condition.value().column == "product_id");
//         REQUIRE(deleteAst->condition.value().op == "=");
//         REQUIRE(deleteAst->condition.value().value == "5");
//     }
// }
//
// // 测试语法错误处理
// TEST_CASE("Syntax error handling", "[parser][error]") {
//     SECTION("Missing semicolon") {
//         std::string sql = "SELECT * FROM users";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast == nullptr); // 解析失败，返回nullptr
//     }
//
//     SECTION("Invalid keyword") {
//         std::string sql = "SELECTT * FROM users;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast == nullptr); // 解析失败，返回nullptr
//     }
//
//     SECTION("Missing FROM clause") {
//         std::string sql = "SELECT * users;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast == nullptr); // 解析失败，返回nullptr
//     }
//
//     SECTION("Invalid column definition") {
//         std::string sql = "CREATE TABLE test (id UNKNOWN_TYPE);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast == nullptr); // 解析失败，返回nullptr
//     }
//
//     SECTION("Unclosed parenthesis") {
//         std::string sql = "INSERT INTO users VALUES (123;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast == nullptr); // 解析失败，返回nullptr
//     }
// }
//
// // 测试复杂SQL语句组合
// TEST_CASE("Complex SQL statement parsing", "[parser][complex]") {
//     SECTION("Complex CREATE TABLE with multiple column types") {
//         std::string sql = "CREATE TABLE employees (id INT, name STRING, department VARCHAR, salary FLOAT);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto createAst = dynamic_cast<CreateTableAST*>(ast.get());
//         REQUIRE(createAst != nullptr);
//         REQUIRE(createAst->tableName == "employees");
//         REQUIRE(createAst->columns.size() == 4);
//     }
//
//     SECTION("Complex INSERT with mixed data types") {
//         std::string sql = "INSERT INTO test VALUES (1, 'test', 3.14);";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto insertAst = dynamic_cast<InsertAST*>(ast.get());
//         REQUIRE(insertAst != nullptr);
//         REQUIRE(insertAst->tableName == "test");
//         REQUIRE(insertAst->values.size() == 3);
//     }
//
//     SECTION("Complex SELECT with multiple columns and WHERE clause") {
//         std::string sql = "SELECT id, name, department FROM employees WHERE salary <= 5000.00;";
//         auto ast = parseSQL(sql);
//
//         REQUIRE(ast != nullptr);
//         auto selectAst = dynamic_cast<SelectAST*>(ast.get());
//         REQUIRE(selectAst != nullptr);
//         REQUIRE(selectAst->tableName == "employees");
//         REQUIRE(selectAst->columns.size() == 3);
//         REQUIRE(selectAst->condition.has_value());
//     }
// }