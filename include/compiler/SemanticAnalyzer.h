#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "AST.h"
#include "../engine/catalog/catalog_manager.h"
#include "../common/Exception.h"
#include "../common/Types.h"
#include <unordered_set>

#include "engine/catalog/schema.h"

namespace minidb {

//语义错误异常类
class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& message) : std::runtime_error(message) {}
};


//语义分析器类，负责对语法分析生成的AST进行语义检查
class SemanticAnalyzer {
public:

    //构造函数
    explicit SemanticAnalyzer(CatalogManager& catalogManager)
           : catalog_manager_(catalogManager) {}

    //语义分析入口函数，根据AST节点类型分发到相应的分析函数
    void analyze(ASTNode* ast);

private:
    CatalogManager& catalog_manager_;  // 目录管理器引用

    //分析CREATE TABLE语句
    void analyzeCreateTable(CreateTableAST* ast);

    //分析INSERT语句
    void analyzeInsert(InsertAST* ast);

    //分析SELECT语句
    void analyzeSelect(SelectAST* ast);

    //分析DELETE语句
    void analyzeDelete(DeleteAST* ast);

    //分析WHERE条件表达式
    void analyzeCondition(const Condition& cond, const Schema& schema, const std::string& table_name);

    //检查运算符是否合法
    bool isValidOperator(const std::string& op);

    //检查运算符与数据类型的兼容性
    bool isOperatorCompatibleWithType(const std::string& op, TypeId type);

    //检查值与类型是否匹配
    bool checkValueMatchType(const std::string& value, const std::string& type);
};

} // namespace minidb

#endif // SEMANTIC_ANALYZER_H