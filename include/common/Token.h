
#ifndef MINIDB_TOKEN_H
#define MINIDB_TOKEN_H

#include "TokenType.h"
#include <string>
#include <utility>
using namespace std;

//Token结构体（存储单个词法单元信息）
struct Token {
    TokenType type;       //单词符号类型（种别码）
    string value;    //单词符号的具体值（如"SELECT"、"name"）
    int line;             //在源代码中的行号（调试用）
    int column;           //列号（调试用）

    //添加构造函数
    Token(TokenType t, string v, int l, int c)
        : type(t), value(std::move(v)), line(l), column(c) {}

};

#endif //MINIDB_TOKEN_H