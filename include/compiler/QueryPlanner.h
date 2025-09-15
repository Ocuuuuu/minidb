//
// Created by tang_ on 2025/9/9.
//

//执行计划生成器

#ifndef MINIDB_QUERYPLANNER_H
#define MINIDB_QUERYPLANNER_H


#include "AST.h"
#include <memory>
#include <string>
#include <vector>

#include "json.hpp"

namespace minidb {

    /**
     * 执行计划生成器：将 AST 转换为逻辑执行计划（JSON 格式）
     */
    class QueryPlanner {
    public:
        /**
         * 构造函数
         */
        QueryPlanner() = default;

        /**
         * 生成执行计划
         * @param ast AST 根节点
         * @return JSON 格式的执行计划
         */
        nlohmann::json generatePlan(ASTNode* ast);

    private:
        /**
         * 处理 CREATE TABLE 语句
         */
        nlohmann::json handleCreateTable(CreateTableAST* ast);

        /**
         * 处理 INSERT 语句
         */
        nlohmann::json handleInsert(InsertAST* ast);

        /**
         * 处理 SELECT 语句
         */
        nlohmann::json handleSelect(SelectAST* ast);

        /**
         * 处理 DELETE 语句
         */
        nlohmann::json handleDelete(DeleteAST* ast);
    };

} // namespace minidb

#endif // MINIDB_QUERYPLANNER_H
