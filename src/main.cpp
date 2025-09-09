#include <iostream>
#include "lexer/Lexer.h"

// 辅助函数：打印Token列表（方便验证结果）
void PrintTokens(const std::vector<Token>& tokens) {
    std::cout << "SQL词法分析结果:\n";
    std::cout << "----------------------------------------\n";
    for (const auto& token : tokens) {
        // 转换TokenType为字符串（方便阅读）
        std::string type_str;
        switch (token.type) {
        case TokenType::KEYWORD: type_str = "关键字"; break;
        case TokenType::ID:      type_str = "标识符"; break;
        case TokenType::NUMBER:  type_str = "数字";   break;
        case TokenType::OP:      type_str = "运算符"; break;
        case TokenType::RANGE:   type_str = "分隔符"; break;
        default:                 type_str = "未知";   break;
        }
        // 输出Token信息
        std::cout << "类型: " << type_str
                  << " | 内容: " << token.value
                  << " | 位置: 行" << token.line << " 列" << token.column << "\n";
    }
}

int main() {
    // 测试SQL语句
    std::string test_sql = R"(
        CREATE TABLE Students (name STRING, age INT);
        INSERT INTO Students VALUES ("Tom", 22);
        SELECT name, age FROM Students WHERE age > 20;
    )";

    // 执行词法分析
    Lexer lexer(test_sql);
    std::vector<Token> tokens = lexer.Tokenize();

    // 打印结果
    PrintTokens(tokens);

    return 0;
}