#ifndef MINIDB_PARSER_H
#define MINIDB_PARSER_H

#include "compiler/AST.h"
#include "compiler/Lexer.h"
#include <stack>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <string>

using namespace std;

//语法分析器类：基于LL(1)文法，将Token序列转换为AST
class Parser {
private:
    Lexer& lexer;//词法分析器（用于获取Token）
    Token currentToken;//当前处理的Token
    stack<string> symStack;//语法栈（存储非终结符/终结符）
    vector<Token> tokens;//缓存所有Token（便于处理）
    size_t tokenPos;//当前Token在tokens中的位置

    //预测分析表：key为"非终结符|当前Token值"，value为产生式（字符串列表）
    unordered_map<string, vector<string>> predictTable;

    //分隔符常量定义（统一管理，避免硬编码错误）
    const string LPAREN = "LPAREN";// 左括号 (
    const string RPAREN = "RPAREN";// 右括号 )
    const string COMMA = "COMMA";// 逗号 ,
    const string SEMICOLON = "SEMICOLON";// 分号 ;
    const string STAR = "STAR";// 星号 *

    //初始化预测分析表（核心！定义LL(1)文法的产生式）
    void initPredictTable();

    // //调试辅助函数
    // void debugString(const string& s, const string& label);
    // void debugState();

    //解析非终结符：根据预测表压入对应产生式
    void parseNonTerminal(const string& nonTerminal);

    //匹配当前Token与预期终结符，匹配成功则获取下一个Token
    void match(const string& expectedValue);

    //关键字处理：确保预测表匹配和Token识别逻辑一致
    string getCurrentTokenKey() const;

    //重置解析器状态（用于重新解析新的SQL语句）
    void resetParser();

    //解析非终结符：Prog（程序入口，对应完整SQL语句）
    unique_ptr<ASTNode> parseProg();

    //解析非终结符：Stmt（语句，可能是CREATE/INSERT/SELECT）
    unique_ptr<ASTNode> parseStmt();

    //解析CREATE TABLE语句
    unique_ptr<CreateTableAST> parseCreateTable();

    //解析列列表（如"name STRING, age INT"）
    vector<Column> parseColumnList();
    bool isNonTerminal(const string& symbol);

    //解析列列表的递归部分（处理逗号分隔的后续列）
    void parseColumnListPrime();

    //解析INSERT语句
    unique_ptr<InsertAST> parseInsert();

    //解析值列表（如"'Alice', 20"）
    vector<string> parseValueList();

    //解析值列表的递归部分（处理逗号分隔的后续值）
    void parseValueListPrime();

    //解析SELECT语句
    unique_ptr<SelectAST> parseSelect();

    //解析DELETE语句
    unique_ptr<DeleteAST> parseDelete();

    //解析查询列列表（如"name, age"或"*"）
    vector<string> parseSelectColumns();

    //解析查询列列表的递归部分（处理逗号分隔的后续列）
    void parseSelectColumnsPrime();

    //解析WHERE子句（可选，如"WHERE age > 20"）
    optional<Condition> parseWhereClause();

    // 新增：存储匹配到的Token值
    std::string matchedValue;

public:
    //构造函数：接收词法分析器，初始化语法栈和预测表
    explicit Parser(Lexer& l);

    //核心方法：执行语法分析，返回AST根节点
    unique_ptr<ASTNode> parse();
};

void printAST(ASTNode* ast);

#endif // MINIDB_PARSER_H