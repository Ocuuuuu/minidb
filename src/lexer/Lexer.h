#ifndef MINIDB_LEXER_H
#define MINIDB_LEXER_H

#include <string>
#include <vector>

// Token类型（对应SQL中的基础元素）
enum class TokenType {
    KEYWORD,   // 关键字：SELECT/FROM/WHERE等
    ID,        // 标识符：表名/列名（如Students/name）
    NUMBER,    // 数字常量（如20）
    OP,        // 运算符：>、=、+等
    RANGE,     // 分隔符：,、;
    UNKNOWN    // 未知符号
};

// Token结构（存储单个词法单元信息）
struct Token {
    TokenType type;       // 类型
    std::string value;    // 内容（如"SELECT"、"name"）
    int line;             // 行号（调试用）
    int column;           // 列号（调试用）

    // 添加构造函数
    Token(TokenType t, const std::string& val, int l, int col)
        : type(t), value(val), line(l), column(col) {}

};

// 词法分析器类（核心功能：将SQL字符串拆分为Token列表）
class Lexer {
private:
    std::string sql_;     // 输入的SQL语句
    std::vector<std::string> keywords_ = {  // 关键字列表
        "SELECT", "FROM", "WHERE", "CREATE", "TABLE", "INSERT", "INTO", "VALUES"
    };

public:
    // 构造函数（传入SQL语句）
    explicit Lexer(std::string sql) : sql_(std::move(sql)) {}

    // 核心方法：执行词法分析，返回Token列表
    std::vector<Token> Tokenize();
};

#endif //MINIDB_LEXER_H