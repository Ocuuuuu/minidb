#include "../../include/compiler/Lexer.h"
#include <cctype>   // 用于判断字符类型（字母/数字/空格）
#include <algorithm> // 用于查找关键字
#include <utility>

//构造函数 初始化词法分析器的SQL字符串、位置、行号、列号，初始化关键字集合
Lexer::Lexer(string s):sql(std::move(s)),pos(0),line(1),column(0)
{
    initKeywords();
}

//初始化关键字集合
void Lexer::initKeywords()
{
    keywords = {"SELECT", "FROM", "WHERE", "CREATE", "TABLE", "INSERT", "INTO", "VALUES"};
}


//跳过空白字符，并更新行号和列号
void Lexer::skipWhitespace()
{
    while (pos<sql.size())
    {
        char c=sql[pos];
        if (c==' '||c=='\t')
        {
            column++;
            pos++;
        }else if (c=='\n')
        {
            line++;
            pos++;
            column=0;
        }else
        {
            break;
        }
    }
}

//处理标识符和关键字
Token Lexer::handleIdentifierOrKeyword()
{
    int startColumn=column;
    size_t startPos=pos;
    //遍历字符 直到遇到非字母/数字/下划线
    while (pos<sql.size()&&(isalnum(sql[pos]) || sql[pos]=='_'))
    {
        pos++;
        column++;
    }
    string word=sql.substr(startPos,pos-startPos);
    //判断是否为关键字
    if (keywords.find(word)!=keywords.end())
    {
        return Token(TokenType::KEYWORD,word,line,startColumn);
    }else
    {
        return Token(TokenType::IDENTIFIER,word,line,startColumn);
    }
}

//处理常数
Token Lexer::handleConstant()
{
    int startColumn=column;
    size_t startPos=pos;
    if (sql[pos]=='"')//字符串常数以双引号开始
    {
        pos++;
        column++;
        //找到双引号结尾
        while (pos<sql.size()&&(sql[pos]!='"'))
        {
            pos++;
            column++;
        }
        if (pos<sql.size()&&sql[pos]=='"'){
            //正确闭合
            pos++;
            column++;
            string str=sql.substr(startPos+1,pos-startPos-2);
            return Token(TokenType::CONSTANT,str,line,startColumn);
        }
        //未正确闭合
        return Token(TokenType::ERROR,"未闭合字符串",line,startColumn);
    }else if (isdigit(sql[pos]))
    {
        //整数常数
        while (pos<sql.size()&&isdigit(sql[pos]))
        {
            pos++;
            column++;
        }
        string num=sql.substr(startPos,pos-startPos);
        return Token(TokenType::CONSTANT,num,line,startColumn);
    }
    //其他类型常数
    return Token(TokenType::IDENTIFIER,sql,line,startColumn);
}

//处理运算符(单字运算符)
Token Lexer::handleOperator()
{
    int startColumn = column;
    size_t startPos = pos;
    if (pos < sql.size()) {
        char op = sql[pos];
        pos++;
        column++;
        string opStr(1, op);
        return Token(TokenType::OPERATOR, opStr, line, startColumn);
    }
    return Token(TokenType::ERROR, "非法运算符", line, startColumn);
}


//处理界符
Token Lexer::handleDelimiter()
{
    int startColumn = column;
    size_t startPos = pos;
    if (pos < sql.size()) {
        char delim = sql[pos];
        pos++;
        column++;
        string delimStr(1, delim);
        return Token(TokenType::DELIMITER, delimStr, line, startColumn);
    }
    return Token(TokenType::ERROR, "非法界符", line, startColumn);
}

//获取下一个Token
Token Lexer::nextToken()
{
    skipWhitespace(); //先跳过空白字符
    if (pos >= sql.size()) { //处理到字符串末尾，返回 EOF_TOKEN
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }
    char c = sql[pos];
    //根据当前字符类型，调用相应的处理方法
    if (isalpha(c) || c == '_') {
        return handleIdentifierOrKeyword();
    } else if (isdigit(c) || c == '"') {
        return handleConstant();
    } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '>' || c == '<') {
        return handleOperator();
    } else if (c == ',' || c == ';' || c == '(' || c == ')') {
        return handleDelimiter();
    } else { //无法识别的字符，视为错误
        pos++;
        column++;
        return Token(TokenType::ERROR, string(1, c), line, column - 1);
    }
}
//获取所有Token 直到遇到EOF_Token
vector<Token> Lexer::getAllTokens()
{
    vector<Token> tokens;
    Token token = nextToken();
    while (token.type != TokenType::EOF_TOKEN) {
        tokens.push_back(token);
        token = nextToken();
    }
    tokens.push_back(token); //加入EOF_TOKEN
    return tokens;
}

