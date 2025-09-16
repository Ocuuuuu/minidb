#include "../../include/compiler/Lexer.h"
#include <cctype>    // 用于判断字符类型（字母/数字/空格）
#include <algorithm> // 用于查找关键字
#include <utility>

// 构造函数：初始化词法分析器的SQL字符串、位置、行号、列号，初始化关键字集合
Lexer::Lexer(std::string s) : sql(std::move(s)), pos(0), line(1), column(0) {
    initKeywords();
}

// 初始化关键字集合（包含SQL常用关键字，区分大小写）
void Lexer::initKeywords() {
    keywords = {
        "SELECT", "FROM", "WHERE", "CREATE", "TABLE",
        "INSERT", "INTO", "VALUES", "INT", "INTEGER",
        "STRING", "VARCHAR", "FLOAT", "DOUBLE", "DELETE", 
        "int", "integer", "string", "varchar", "float", "double", "delete"
    };
}

// 跳过空白字符（空格、制表符、换行符），并更新行号和列号
void Lexer::skipWhitespace() {
    while (pos < sql.size()) {
        char c = sql[pos];
        if (c == ' ' || c == '\t') {
            // 空格和制表符：只增加列号
            column++;
            pos++;
        } else if (c == '\n') {
            // 换行符：行号+1，列号重置为0
            line++;
            pos++;
            column = 0;
        } else {
            // 非空白字符：停止跳过
            break;
        }
    }
}

// 处理标识符和关键字（以字母或下划线开头，后续可跟字母、数字或下划线）
Token Lexer::handleIdentifierOrKeyword() {
    int startColumn = column;  // 记录起始列号（用于错误定位）
    size_t startPos = pos;     // 记录起始位置（用于截取字符串）

    // 遍历字符直到遇到非字母/数字/下划线
    while (pos < sql.size() && (isalnum(sql[pos]) || sql[pos] == '_')) {
        pos++;
        column++;
    }

    // 截取从起始位置到当前位置的字符串
    std::string word = sql.substr(startPos, pos - startPos);
    std::string upperWord = word;
    std::transform(upperWord.begin(), upperWord.end(), upperWord.begin(), ::toupper);

    if (keywords.find(upperWord) != keywords.end()) {
        return Token(TokenType::KEYWORD, word, line, startColumn);
    } else {
        return Token(TokenType::IDENTIFIER, word, line, startColumn);
    }
}

// 处理常数（扩展支持：整数、浮点数、单双引号字符串）
Token Lexer::handleConstant() {

    int startColumn = column;
    size_t startPos = pos;

    // 处理字符串常量（单引号或双引号）
    if (sql[pos] == '\"' || sql[pos] == '\'') {
        char quoteChar = sql[pos];
        pos++; // 跳过开头的引号
        column++;

        std::string str; // 不包含引号的字符串内容
        while (pos < sql.size()) {
            if (sql[pos] == '\\' && pos + 1 < sql.size()) {
                // 处理转义字符（如 \" 或 \'）
                str += sql[pos + 1]; // 直接添加转义后的字符（忽略反斜杠）
                pos += 2;
                column += 2;
            } else if (sql[pos] != quoteChar) {
                str += sql[pos]; // 添加普通字符
                pos++;
                column++;
            } else {
                break; // 遇到闭合引号
            }
        }

        if (pos < sql.size() && sql[pos] == quoteChar) {
            pos++; // 跳过结尾的引号
            column++;
            return Token(TokenType::CONSTANT, str, line, startColumn); // 返回不包含引号的字符串
        } else {
            return Token(TokenType::ERROR, "Unclosed string", line, startColumn);
        }
    }

    // 处理数值常量（整数或浮点数）
    else if (isdigit(static_cast<unsigned char>(sql[pos])) || sql[pos] == '.') {
        size_t numStart = pos;
        while (pos < sql.size()) {
            char c = sql[pos];
            if (isdigit(static_cast<unsigned char>(c)) || c == '.') {
                pos++;
                column++;
            } else {
                break;
            }
        }

        std::string numStr = sql.substr(numStart, pos - numStart);
        int dotCount = std::count(numStr.begin(), numStr.end(), '.');
        bool hasDigit = std::any_of(numStr.begin(), numStr.end(), [](unsigned char c) { return std::isdigit(c); });

        if (dotCount > 1 || !hasDigit) {
            return Token(TokenType::ERROR, "Invalid number format (" + numStr + ")", line, startColumn);
        }

        return Token(TokenType::CONSTANT, numStr, line, startColumn);
    }

    // 未知常量类型
    return Token(TokenType::ERROR, "Unknown constant type", line, startColumn);
}

// 处理运算符
Token Lexer::handleOperator() {
    int startColumn = column;
    char current = sql[pos];

    // 优先判断双字符运算符（顺序很重要）
    if (pos + 1 < sql.size()) {
        char next = sql[pos + 1];
        std::string op_str;
        op_str += current;
        op_str += next;

        // 明确列出所有双字符运算符，确保覆盖
        const std::unordered_set<std::string> double_ops = {"==", "!=", ">=", "<="};
        if (double_ops.count(op_str)) {
            pos += 2;
            column += 2;
            return Token(TokenType::OPERATOR, op_str, line, startColumn);
        }
    }

    // 处理单字符运算符
    const std::unordered_set<char> single_ops = {'+', '-', '*', '/', '%', '=', '>', '<', '!'};
    if (single_ops.count(current)) {
        pos++;
        column++;
        return Token(TokenType::OPERATOR, std::string(1, current), line, startColumn);
    }

    // 无效运算符
    pos++;
    column++;
    return Token(TokenType::ERROR, "Unrecognized operator (" + std::string(1, current) + ")", line, startColumn);
}

// 处理界符（逗号、分号、括号等）
Token Lexer::handleDelimiter() {
    int startColumn = column;
    // size_t startPos = pos;

    if (pos < sql.size()) {
        char delim = sql[pos];
        pos++;
        column++;
        std::string delimStr(1, delim);
        return Token(TokenType::DELIMITER, delimStr, line, startColumn);
    }

    // 非法界符：返回错误
    return Token(TokenType::ERROR, "Invalid delimiter", line, startColumn);
}

Token Lexer::nextToken() {
    skipWhitespace();

    if (pos >= sql.size()) {
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }

    char c = sql[pos];

    if (isalpha(c) || c == '_') {
        return handleIdentifierOrKeyword();
    }
    // 关键修复：加入 c == '.'，确保小数点进入常量处理
    else if (isdigit(c) || c == '.' || c == '"' || c == '\'') {
        return handleConstant();
    }
    else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
             c == '=' || c == '>' || c == '<' || c == '!') {
        return handleOperator();
             }
    else if (c == ',' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}') { // 补充 } 界符
        return handleDelimiter();
    }
    else {
        // 修复：中文→英文错误信息
        pos++;
        column++;
        return Token(TokenType::ERROR, "Unrecognized character (" + std::string(1, c) + ")", line, column - 1);
    }
}

// 获取所有Token（直到遇到EOF）
vector<Token> Lexer::getAllTokens() {
    vector<Token> tokens;
    // 直接获取第一个Token，无需默认构造
    Token current_token = nextToken();

    // 遍历所有非EOF的Token，添加到列表中
    while (current_token.type != TokenType::EOF_TOKEN) {
        tokens.push_back(current_token);
        current_token = nextToken();  // 获取下一个Token
    }

    // 最后添加EOF_TOKEN，标记Token序列结束
    tokens.push_back(current_token);
    return tokens;
}


