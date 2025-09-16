//
// QueryPlanner.cpp
// 执行计划生成器实现
//

#include "compiler/QueryPlanner.h"

#include "compiler/AST.h"
#include <sstream>
#include <iomanip>
#include "../../include/compiler/PlanNode.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

namespace minidb {

json QueryPlanner::generatePlan(ASTNode* ast) {
    if (!ast) {
        throw std::runtime_error("AST is null");
    }

    // 根据 AST 类型分发处理逻辑
    if (auto createTableAst = dynamic_cast<CreateTableAST*>(ast)) {
        return handleCreateTable(createTableAst);
    } else if (auto insertAst = dynamic_cast<InsertAST*>(ast)) {
        return handleInsert(insertAst);
    } else if (auto selectAst = dynamic_cast<SelectAST*>(ast)) {
        return handleSelect(selectAst);
    } else if (auto deleteAst = dynamic_cast<DeleteAST*>(ast)) {
        return handleDelete(deleteAst);
    } else {
        throw std::runtime_error("Unsupported AST node type");
    }
}

json QueryPlanner::handleCreateTable(CreateTableAST* ast) {
    json plan;
    plan["type"] = "CreateTable";
    plan["tableName"] = ast->tableName;

    // 处理列定义
    json columns;
    for (const auto& col : ast->columns) {
        json column;
        column["name"] = col.name;
        column["type"] = col.type;
        columns.push_back(column);
    }
    plan["columns"] = columns;

    return plan;
}

json QueryPlanner::handleInsert(InsertAST* ast) {
    json plan;
    plan["type"] = "Insert";
    plan["tableName"] = ast->tableName;
    plan["values"] = ast->values;

    return plan;
}

    json QueryPlanner::handleSelect(SelectAST* ast) {
    json plan;

    // 1. 生成 SeqScan 节点
    json seqScan;
    seqScan["type"] = "SeqScan";
    seqScan["tableName"] = ast->tableName;



    // 2. 如果有 WHERE 条件，生成 Filter 节点
    if (ast->condition) {
        json filter;
        filter["type"] = "Filter";
        filter["condition"] = {
            {"column", ast->condition->column},
            {"op", ast->condition->op},
            {"value", ast->condition->value}
        };
        filter["input"] = seqScan;
        plan = filter;
    } else {
        plan = seqScan;
    }

    // 3. 生成 Project 节点（处理查询列）
    json project;
    project["type"] = "Project";
    project["columns"] = ast->columns;
    project["input"] = plan;
    plan = project;

    return plan;
}

    json QueryPlanner::handleDelete(DeleteAST* ast) {
    json plan;

    // 1. 生成 SeqScan 节点
    json seqScan;
    seqScan["type"] = "SeqScan";
    seqScan["tableName"] = ast->tableName;

    // 2. 如果有 WHERE 条件，生成 Filter 节点
    if (ast->condition) {
        json filter;
        filter["type"] = "Filter";
        filter["condition"] = {
            {"column", ast->condition->column},
            {"op", ast->condition->op},
            {"value", ast->condition->value}
        };
        filter["input"] = seqScan;
        plan = filter;
    } else {
        throw std::runtime_error("DELETE without WHERE is not supported");
    }

    // 3. 生成 Delete 节点
    json deleteNode;
    deleteNode["type"] = "Delete";
    deleteNode["tableName"] = ast->tableName;
    deleteNode["input"] = plan;
    plan = deleteNode;

    return plan;
}
} // namespace minidb


