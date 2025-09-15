//
//抽象语法树，表示SQL语句的语法结构
//

#ifndef MINIDB_AST_H
#define MINIDB_AST_H

#include "../common/Token.h"
#include <vector>
#include <optional>
using namespace std;

//AST节点基类（所有的父类）
class ASTNode {
public:
    virtual ~ASTNode() = default; //虚析构函数，确保子类正确释放
};

//列信息（用于CREATE TABLE和INSERT语句）
struct Column {
    string name;    //列名（如 "age"）
    string type;    //列类型（如 "INT"、"STRING"）
};

//条件表达式（用于WHERE子句，如 "age > 20"）
struct Condition {
    string column;  //列名（如 "age"）
    string op;      //运算符（如 ">"、"="）
    string value;   //值（如 "20"）
};

//CREATE TABLE语句的AST节点
class CreateTableAST : public ASTNode {
public:
    string tableName;               //表名（如 "Students"）
    vector<Column> columns;         //列列表（如 [{"name", "STRING"}, {"age", "INT"}]）
};

//INSERT语句的AST节点
class InsertAST : public ASTNode {
public:
    string tableName;               //表名（如 "Students"）
    vector<string> values;          //插入的值列表（如 ["Alice", "20"]）
};

//SELECT语句的AST节点
class SelectAST : public ASTNode {
public:
    vector<string> columns;         //查询的列（如 ["name", "age"] 或 ["*"]）
    string tableName;               //表名（如 "Students"）
    optional<Condition> condition;  //WHERE 条件（可选，用 optional 表示可能不存在）
};

//DELETE语句的AST节点
class DeleteAST : public ASTNode {
public:
    string tableName;               //表名（如 "Students"）
    optional<Condition> condition;  //WHERE 条件（可选，用 optional 表示可能不存在）
};
#endif //MINIDB_AST_H