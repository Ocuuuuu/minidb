//
// Created by tang_ on 2025/9/16.
//

#ifndef MINIDB_PLANNODE_H
#define MINIDB_PLANNODE_H

#include <json.hpp>
#include <string>
#include <vector>

namespace minidb {

    /**
     * 逻辑执行计划节点基类
     */
    struct PlanNode {
        virtual ~PlanNode() = default;
        virtual nlohmann::json toJson() const = 0;
    };

} // namespace minidb

#endif // MINIDB_PLANNODE_H
