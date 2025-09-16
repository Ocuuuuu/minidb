#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "AST.h"
#include "../engine/catalog/catalog_manager.h"
#include "../common/Exception.h"
#include "../common/Types.h"
#include <unordered_set>

#include "engine/catalog/schema.h"

namespace minidb {

/**
 * 语义错误异常类
 */
class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& message) : std::runtime_error(message) {}
};

/**
 * 语义分析器类
 * 负责对语法分析生成的AST进行语义检查
 * 包括：类型检查、名称解析、作用域检查等
 */
class SemanticAnalyzer {
public:
    /**
     * 构造函数
     * @param catalog_manager 目录管理器引用，用于查询表和列信息
     */
    explicit SemanticAnalyzer(CatalogManager& catalog_manager) 
        : catalog_manager_(catalog_manager) {}

    /**
     * 语义分析入口函数
     * 根据AST节点类型分发到相应的分析函数
     * @param ast AST根节点
     * @throws SemanticError 语义分析失败时抛出
     */
    void analyze(ASTNode* ast);



private:
    CatalogManager& catalog_manager_;  // 目录管理器引用

    /**
     * 分析CREATE TABLE语句
     * 检查：表是否已存在、列名是否重复、列类型是否合法
     * @param ast CreateTableAST节点
     * @throws SemanticError 分析失败时抛出
     */
    void analyzeCreateTable(CreateTableAST* ast);

    /**
     * 分析INSERT语句
     * 检查：表是否存在、插入值数量与列数量是否匹配、值类型与列类型是否匹配
     * @param ast InsertAST节点
     * @throws SemanticError 分析失败时抛出
     */
    void analyzeInsert(InsertAST* ast);

    /**
     * 分析SELECT语句
     * 检查：表是否存在、查询列是否存在、条件表达式是否合法
     * @param ast SelectAST节点
     * @throws SemanticError 分析失败时抛出
     */
    void analyzeSelect(SelectAST* ast);

    /**
     * 分析DELETE语句
     * 检查：表是否存在、WHERE条件是否合法
     * @param ast DeleteAST节点
     * @throws SemanticError 分析失败时抛出
     */
    void analyzeDelete(DeleteAST* ast);

    /**
     * 分析WHERE条件表达式
     * 检查：条件列是否存在、条件值类型是否与列类型匹配、运算符是否合法
     * @param cond 条件对象
     * @param schema 表结构
     * @param table_name 表名（用于错误信息）
     * @throws SemanticError 分析失败时抛出
     */
    void analyzeCondition(const Condition& cond, const Schema& schema, const std::string& table_name);

    /**
     * 检查运算符是否合法
     * @param op 运算符
     * @return 合法返回true，否则返回false
     */
    bool isValidOperator(const std::string& op);

    /**
     * 检查运算符与数据类型的兼容性
     * @param op 运算符
     * @param type 数据类型
     * @return 兼容返回true，否则返回false
     */
    bool isOperatorCompatibleWithType(const std::string& op, TypeId type);

    /**
     * 检查值与类型是否匹配
     * @param value 要检查的值
     * @param type 目标类型（"INT"、"STRING"、"FLOAT"、"BOOLEAN"）
     * @return 匹配返回true，否则返回false
     */
    bool checkValueMatchType(const std::string& value, const std::string& type);
};

} // namespace minidb

#endif // SEMANTIC_ANALYZER_H