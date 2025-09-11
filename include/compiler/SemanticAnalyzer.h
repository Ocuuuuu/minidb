//
// Created by tang_ on 2025/9/9.
//
#pragma once

#include <string>
#include <stdexcept>
#include "compiler/AST.h"
#include "engine/catalog/catalog_manager.h"

using namespace std;

namespace minidb {

    // 语义错误异常类，用于抛出语义分析过程中的错误
    class SemanticError : public runtime_error {
    public:
        explicit SemanticError(const string& msg) : runtime_error(msg) {}
    };

    // 语义分析器类，负责对AST进行语义检查
    class SemanticAnalyzer {
    private:
        // 目录管理器引用，用于获取表的元数据
        CatalogManager& catalog_manager_;

    public:
        // 构造函数，初始化目录管理器引用
        explicit SemanticAnalyzer(CatalogManager& catalog_manager)
            : catalog_manager_(catalog_manager) {}

        // 语义分析入口方法，根据AST节点类型调用不同的分析方法
        void analyze(ASTNode* ast);

    private:
        // 分析CREATE TABLE语句的语义
        void analyzeCreateTable(CreateTableAST* ast);
        // 分析INSERT语句的语义
        void analyzeInsert(InsertAST* ast);
        // 分析SELECT语句的语义
        void analyzeSelect(SelectAST* ast);
        // 检查值与列类型是否匹配
        bool checkValueMatchType(const string& value, const string& type);
    };

} // namespace minidb