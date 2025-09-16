// // Created by tang_ on 2025/9/12.
// //
//
// // 引入 Catch2 测试框架头文件
// #include <iostream>
//
// #include "../catch2/catch_amalgamated.hpp"
//
// #include "../../include/compiler/Lexer.h"
// #include "../../include/common/Token.h"
//
// // 将TokenType转换为字符串，用于调试输出
// static std::string tokenTypeToString(TokenType type) {
//     switch (type) {
//     case TokenType::KEYWORD: return "KEYWORD";
//     case TokenType::IDENTIFIER: return "IDENTIFIER";
//     case TokenType::CONSTANT: return "CONSTANT";
//     case TokenType::OPERATOR: return "OPERATOR";
//     case TokenType::DELIMITER: return "DELIMITER";
//     case TokenType::ERROR: return "ERROR";
//     case TokenType::EOF_TOKEN: return "EOF";
//     default: return "UNKNOWN";
//     }
// }
//
// // 将Token列表转换为字符串表示，用于调试和测试
// static std::string tokensToString(const std::vector<Token>& tokens) {
//     std::string result;
//     for (const auto& token : tokens) {
//         result += "[" + tokenTypeToString(token.type) + ":" + token.value + "]";
//     }
//     return result;
// }
//
// // 测试关键字识别
// TEST_CASE("Keyword recognition", "[lexer][keyword]") {
//     SECTION("Uppercase keywords") {
//         // 包含15个关键字（包括DELETE），注意空格分隔会被解析为15个单独的关键字
//         Lexer lexer("SELECT FROM WHERE CREATE TABLE INSERT INTO VALUES INT INTEGER STRING VARCHAR FLOAT DOUBLE DELETE");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：15个关键字 + 1个EOF = 16个Token
//         REQUIRE(tokens.size() == 16);
//         REQUIRE(tokens[0].type == TokenType::KEYWORD);
//         REQUIRE(tokens[0].value == "SELECT");
//         REQUIRE(tokens[1].value == "FROM");
//         REQUIRE(tokens[8].value == "INT");
//         REQUIRE(tokens[14].value == "DELETE");
//         REQUIRE(tokens[15].type == TokenType::EOF_TOKEN);  // ���һ����EOF
//     }
//
//     SECTION("Lowercase keywords") {
//         // 包含7个小写关键字（包括delete）
//         Lexer lexer("int integer string varchar float double delete");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：7个关键字 + 1个EOF = 8个Token
//         REQUIRE(tokens.size() == 8);
//         REQUIRE(tokens[0].type == TokenType::KEYWORD);
//         REQUIRE(tokens[0].value == "int");
//         REQUIRE(tokens[4].value == "float");
//         REQUIRE(tokens[6].value == "delete");
//     }
// }
//
// // 测试标识符识别
// TEST_CASE("Identifier recognition", "[lexer][identifier]") {
//     SECTION("Valid identifiers") {
//         // 包含4个有效标识符
//         Lexer lexer("user_name age123 _id name_");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：4个标识符 + 1个EOF = 5个Token
//         REQUIRE(tokens.size() == 5);
//         REQUIRE(tokens[0].type == TokenType::IDENTIFIER);
//         REQUIRE(tokens[0].value == "user_name");
//         REQUIRE(tokens[1].value == "age123");
//         REQUIRE(tokens[2].value == "_id");
//     }
//
//     SECTION("Mixed with keywords") {
//         // 包含2个关键字 + 2个标识符
//         Lexer lexer("SELECT select_var FROM table1");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：4个有效Token + 1个EOF = 5个Token
//         REQUIRE(tokens.size() == 5);
//         REQUIRE(tokens[0].type == TokenType::KEYWORD);      // SELECT
//         REQUIRE(tokens[1].type == TokenType::IDENTIFIER);   // select_var
//         REQUIRE(tokens[2].type == TokenType::KEYWORD);      // FROM
//     }
// }
//
// // 测试常量识别
// TEST_CASE("Constant recognition", "[lexer][constant]") {
//     // 测试字符串常量（双引号）
//     SECTION("String constants with double quotes") {
//         Lexer lexer("\"hello\" \"world 123\" \"with\\\"quote\"");
//         auto tokens = lexer.getAllTokens();
//
//         REQUIRE(tokens.size() == 4);
//         REQUIRE(tokens[0].value == "hello");       // 不包含双引号
//         REQUIRE(tokens[1].value == "world 123");   // 不包含双引号
//         REQUIRE(tokens[2].value == "with\"quote"); // 转义引号被正确处理
//     }
//
//     // 测试字符串常量（单引号）
//     SECTION("String constants with single quotes") {
//         Lexer lexer("'hello' 'world 123' 'with\\'quote'");
//         auto tokens = lexer.getAllTokens();
//
//         REQUIRE(tokens.size() == 4);
//         REQUIRE(tokens[0].value == "hello");       // 不包含单引号
//         REQUIRE(tokens[1].value == "world 123");   // 不包含单引号
//         REQUIRE(tokens[2].value == "with'quote");  // 转义引号被正确处理
//     }
//
//     // 测试混合常量和界符
//     SECTION("Mixed constants and delimiters") {
//         Lexer lexer("\"Alice\", 20, 95.5");
//         auto tokens = lexer.getAllTokens();
//         std::string result = tokensToString(tokens);
//
//         REQUIRE(result.find("[CONSTANT:Alice]") != std::string::npos); // 不包含双引号
//         REQUIRE(result.find("[DELIMITER:,]") != std::string::npos);
//         REQUIRE(result.find("[CONSTANT:20]") != std::string::npos);
//         REQUIRE(result.find("[CONSTANT:95.5]") != std::string::npos);
//     }
// }
//
// // 测试运算符识别
// TEST_CASE("Operator recognition", "[lexer][operator]") {
//     SECTION("Single character operators") {
//         // 包含8个单字符运算符
//         Lexer lexer("+ - * / % = > < !");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：9个运算符 + 1个EOF = 10个Token
//         REQUIRE(tokens.size() == 10); // 原预期为9，现为10
//         REQUIRE(tokens[0].type == TokenType::OPERATOR);
//         REQUIRE(tokens[0].value == "+");
//         REQUIRE(tokens[5].value == "=");
//         REQUIRE(tokens[8].value == "!"); // 验证第9个运算符
//         REQUIRE(tokens[9].type == TokenType::EOF_TOKEN); // 验证EOF
//     }
//
//     SECTION("Double character operators") {
//         // 包含4个双字符运算符
//         Lexer lexer("== != >= <=");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：4个运算符 + 1个EOF = 5个Token
//         REQUIRE(tokens.size() == 5);
//         REQUIRE(tokens[0].value == "==");
//         REQUIRE(tokens[1].value == "!=");
//         REQUIRE(tokens[2].value == ">=");
//         REQUIRE(tokens[3].value == "<=");
//     }
// }
//
// // 测试界符识别
// TEST_CASE("Delimiter recognition", "[lexer][delimiter]") {
//     // 包含6个界符
//     Lexer lexer(", ; ( ) { }");
//     auto tokens = lexer.getAllTokens();
//
//     // 预期：6个界符 + 1个EOF = 7个Token
//     REQUIRE(tokens.size() == 7);
//     REQUIRE(tokens[0].type == TokenType::DELIMITER);
//     REQUIRE(tokens[0].value == ",");
//     REQUIRE(tokens[1].value == ";");
//     REQUIRE(tokens[2].value == "(");
//     REQUIRE(tokens[3].value == ")");
// }
//
// // 测试错误处理
// TEST_CASE("Error handling", "[lexer][error]") {
//     SECTION("Unclosed string") {
//         // 包含1个未闭合的字符串
//         Lexer lexer("\"unclosed string");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：1个错误Token + 1个EOF = 2个Token
//         REQUIRE(tokens.size() == 2);
//         REQUIRE(tokens[0].type == TokenType::ERROR);
//         REQUIRE(tokens[0].value.find("Unclosed string") != std::string::npos); // 用英文匹配
//     }
//
//     // 测试无效数值格式（确保用英文匹配）
//     SECTION("Invalid number format") {
//         Lexer lexer(". 123..45");
//         auto tokens = lexer.getAllTokens();
//         std::cout << "Invalid tokens: " << tokensToString(tokens) << std::endl; // 调试输出
//         REQUIRE(tokens.size() == 3);
//         REQUIRE(tokens[0].value.find("Invalid number format") != std::string::npos); // 英文匹配
//     }
//
//     SECTION("Unknown characters") {
//         // 包含5个未识别的字符
//         Lexer lexer("@ # $ ^ &");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：5个错误Token + 1个EOF = 6个Token
//         REQUIRE(tokens.size() == 6);
//         REQUIRE(tokens[0].type == TokenType::ERROR);
//         // 匹配实际返回的错误信息：Unrecognized character
//         REQUIRE(tokens[0].value.find("Unrecognized character (@)") != std::string::npos);
//     }
// }
//
// // 测试空白字符处理
// TEST_CASE("Whitespace handling", "[lexer][whitespace]") {
//     SECTION("Spaces and tabs") {
//         // 包含2个关键字 + 2个标识符，带有多余空格和制表符
//         Lexer lexer("SELECT   name\tFROM  users");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：4个有效Token + 1个EOF = 5个Token
//         REQUIRE(tokens.size() == 5);
//         REQUIRE(tokens[0].value == "SELECT");
//         REQUIRE(tokens[1].value == "name");
//         REQUIRE(tokens[2].value == "FROM");
//     }
//
//     SECTION("New lines") {
//         // 包含2个关键字 + 2个标识符，带有换行符
//         Lexer lexer("SELECT\nname\nFROM\nusers");
//         auto tokens = lexer.getAllTokens();
//
//         // 预期：4个有效Token + 1个EOF = 5个Token
//         REQUIRE(tokens.size() == 5);
//         REQUIRE(tokens[0].line == 1);
//         REQUIRE(tokens[1].line == 2);  // 验证行号变化
//         REQUIRE(tokens[2].line == 3);
//     }
// }
//
// // 测试复杂SQL语句
// TEST_CASE("Complex SQL statements", "[lexer][integration]") {
//     SECTION("CREATE TABLE statement") {
//         std::string sql = "CREATE TABLE students ("
//                           "id INT, "
//                           "name VARCHAR, "
//                           "age INT, "
//                           "score FLOAT"
//                           ");";
//         Lexer lexer(sql);
//         auto tokens = lexer.getAllTokens();
//
//         // 验证关键Token序列，不需要详细验证所有Token，只验证关键信息
//         REQUIRE(tokensToString(tokens).find("[KEYWORD:CREATE][KEYWORD:TABLE][IDENTIFIER:students][DELIMITER:(]") != std::string::npos);
//         REQUIRE(tokensToString(tokens).find("[IDENTIFIER:id][KEYWORD:INT][DELIMITER:,]") != std::string::npos);
//         REQUIRE(tokensToString(tokens).find("[IDENTIFIER:score][KEYWORD:FLOAT][DELIMITER:)][DELIMITER:;]") != std::string::npos);
//     }
//
//     SECTION("INSERT statement") {
//         std::string sql = "INSERT INTO students VALUES "
//                           "('Alice', 20, 95.5), "
//                           "('Bob', 21, 88.0);";
//         Lexer lexer(sql);
//         auto tokens = lexer.getAllTokens();
//
//         REQUIRE(tokensToString(tokens).find("[KEYWORD:INSERT][KEYWORD:INTO][IDENTIFIER:students][KEYWORD:VALUES]") != std::string::npos);
//         REQUIRE(tokensToString(tokens).find("[CONSTANT:Alice][DELIMITER:,][CONSTANT:20][DELIMITER:,][CONSTANT:95.5]") != std::string::npos);
//     }
//
//     SECTION("SELECT statement with WHERE clause") {
//         std::string sql = "SELECT name, age FROM students "
//                           "WHERE age >= 18 AND score > 90.0;";
//         Lexer lexer(sql);
//         auto tokens = lexer.getAllTokens();
//
//         REQUIRE(tokensToString(tokens).find("[KEYWORD:SELECT][IDENTIFIER:name][DELIMITER:,][IDENTIFIER:age][KEYWORD:FROM]") != std::string::npos);
//         REQUIRE(tokensToString(tokens).find("[KEYWORD:WHERE][IDENTIFIER:age][OPERATOR:>=][CONSTANT:18]") != std::string::npos);
//         REQUIRE(tokensToString(tokens).find("[IDENTIFIER:score][OPERATOR:>][CONSTANT:90.0][DELIMITER:;]") != std::string::npos);
//     }
//
//     SECTION("DELETE statement") {
//         std::string sql = "DELETE FROM students WHERE age < 18;";
//         Lexer lexer(sql);
//         auto tokens = lexer.getAllTokens();
//
//         REQUIRE(tokensToString(tokens).find("[KEYWORD:DELETE][KEYWORD:FROM][IDENTIFIER:students][KEYWORD:WHERE]") != std::string::npos);
//         REQUIRE(tokensToString(tokens).find("[IDENTIFIER:age][OPERATOR:<][CONSTANT:18][DELIMITER:;]") != std::string::npos);
//     }
// }
