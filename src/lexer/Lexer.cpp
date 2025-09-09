#include "Lexer.h"
#include <cctype>   // 用于判断字符类型（字母/数字/空格）
#include <algorithm> // 用于查找关键字

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;  // 存储最终Token列表
    int pos = 0;                // 当前字符位置
    int line = 1;               // 当前行号
    int column = 1;             // 当前列号
    int sql_len = sql_.size();  // SQL语句长度

    while (pos < sql_len) {
        char c = sql_[pos];

        // 1. 跳过空格/换行（更新行号和列号）
        if (isspace(c)) {
            if (c == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            pos++;
            continue;
        }

        // 2. 识别标识符/关键字（字母/下划线开头）
        if (isalpha(c) || c == '_') {
            int start_pos = pos;
            // 读取连续的字母/数字/下划线
            while (pos < sql_len && (isalnum(sql_[pos]) || sql_[pos] == '_')) {
                pos++;
                column++;
            }
            // 提取字符串，判断是否为关键字
            std::string value = sql_.substr(start_pos, pos - start_pos);
            TokenType type = TokenType::ID;
            if (std::find(keywords_.begin(), keywords_.end(), value) != keywords_.end()) {
                type = TokenType::KEYWORD;
            }
            // 添加到Token列表
            tokens.emplace_back(type, value, line, column - (pos - start_pos));
            continue;
        }

        // 3. 识别数字常量（0-9开头）
        if (isdigit(c)) {
            int start_pos = pos;
            // 读取连续的数字
            while (pos < sql_len && isdigit(sql_[pos])) {
                pos++;
                column++;
            }
            std::string value = sql_.substr(start_pos, pos - start_pos);
            tokens.emplace_back(TokenType::NUMBER, value, line, column - (pos - start_pos));
            continue;
        }

        // 4. 识别运算符（>、<、=、!=、>=、<=）
        if (c == '>' || c == '<' || c == '=' || c == '!') {
            std::string value(1, c);
            // 检查是否为双字符运算符（如>=、!=）
            if (pos + 1 < sql_len && sql_[pos + 1] == '=') {
                value += '=';
                pos++;
                column++;
            }
            tokens.emplace_back(TokenType::OP, value, line, column);
            pos++;
            column++;
            continue;
        }

        // 5. 识别分隔符（,、;、.）
        if (c == ',' || c == ';' || c == '.') {
            tokens.emplace_back(TokenType::RANGE, std::string(1, c), line, column);
            pos++;
            column++;
            continue;
        }

        // 6. 未知符号（如#、@）
        tokens.emplace_back(TokenType::UNKNOWN, std::string(1, c), line, column);
        pos++;
        column++;
    }

    return tokens;
}