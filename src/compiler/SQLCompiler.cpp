//
// Created by tang_ on 2025/9/16.
//封装四个方法

#include "../../include/compiler/SQLCompiler.h"

#include <iostream>

#include "../../include/compiler/AST.h"

namespace minidb {

    SQLCompiler::SQLCompiler(CatalogManager& catalog) : catalogManager(catalog) {}

    std::unordered_map<std::string, std::string> SQLCompiler::compile(const std::string& sql) {

    // 1. 词法分析
    Lexer lexer(sql);
    auto tokens = lexer.getAllTokens();
    if (tokens.empty()) {
        throw std::runtime_error("Lexer error: No tokens generated.");
    }
    // 打印 Token 序列
    std::cerr << "Tokens: ";
    for (const auto& token : tokens) {
        std::cerr << token.value << " ";
    }
    std::cerr << std::endl;

    // 2. 语法分析
    Parser parser(lexer);
    auto ast = parser.parse();
    if (!ast) {
        throw std::runtime_error("Parser error: Failed to generate AST.");
    }

    // 3. 语义分析
    SemanticAnalyzer analyzer(catalogManager);
    analyzer.analyze(ast.get());

    // 4. 执行计划生成
    QueryPlanner planner;
    auto plan = planner.generatePlan(ast.get());

    // 根据AST类型补充元信息
    if (dynamic_cast<CreateTableAST*>(ast.get())) {
        plan["astType"] = "CreateTable";
        // 调用CatalogManager创建表
        auto createAst = static_cast<CreateTableAST*>(ast.get());
        catalogManager.create_table_from_ast(*createAst);
    }
    else if (dynamic_cast<InsertAST*>(ast.get())) {
        plan["astType"] = "Insert";
        // 验证表是否存在
        auto insertAst = static_cast<InsertAST*>(ast.get());
        if (!catalogManager.table_exists(insertAst->tableName)) {
            throw std::runtime_error("Table '" + insertAst->tableName + "' does not exist.");
        }
    }
    else if (dynamic_cast<SelectAST*>(ast.get())) {
        plan["astType"] = "Select";
        // 验证表是否存在
        auto selectAst = static_cast<SelectAST*>(ast.get());
        if (!catalogManager.table_exists(selectAst->tableName)) {
            throw std::runtime_error("Table '" + selectAst->tableName + "' does not exist.");
        }
    }
    else if (dynamic_cast<DeleteAST*>(ast.get())) {
        plan["astType"] = "Delete";
        // 验证表是否存在
        auto deleteAst = static_cast<DeleteAST*>(ast.get());
        if (!catalogManager.table_exists(deleteAst->tableName)) {
            throw std::runtime_error("Table '" + deleteAst->tableName + "' does not exist.");
        }
    }

    return plan;
}

} // namespace minidb