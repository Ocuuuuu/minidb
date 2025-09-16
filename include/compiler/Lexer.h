#ifndef MINIDB_LEXER_H
#define MINIDB_LEXER_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/Token.h"
#include "common/TokenType.h"
using namespace std;

//词法分析器类（将SQL字符串拆分为Token序列）
class Lexer {
private:
    string sql;     //输入的SQL语句
    size_t pos;//当前处理到的符号位置
    int line;//当前行号
    int column;//列号
    unordered_set<string> keywords;//预定义的关键字集合

    //初始化关键字集合
    void initKeywords();
    //跳过空白字符，并更新行号和列号
    void skipWhitespace();
    //处理标识符和关键字
    Token handleIdentifierOrKeyword();
    //处理常数
    Token handleConstant();
    //处理运算符
    Token handleOperator();
    //处理界符
    Token handleDelimiter();

public:
    //构造函数（传入SQL语句）
    explicit Lexer(string s);
    //获取下一个Token
    Token nextToken();
    //获取所有Token 直到遇到EOF_Token
    vector<Token> getAllTokens();

};

#endif //MINIDB_LEXER_H