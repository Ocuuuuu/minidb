//
// Created by 罗飞燕 on 2025/9/10.
//

#include "../include/engine/catalog/table_info.h"
#include <functional> // 用于std::hash

namespace minidb {

    /**
     * @brief TableInfo构造函数实现
     *
     * 使用表名和模式初始化表信息，并生成一个简单的表ID。
     * 当前使用表名的哈希值作为ID，实际系统中可能需要更复杂的ID生成机制。
     */
    //具体待定，需要修改和确定
    TableInfo::TableInfo(const std::string& table_name, const Schema& schema)
        : table_name_(table_name), schema_(schema),
          table_id_(std::hash<std::string>{}(table_name)) {
        // 使用表名的哈希值作为临时表ID
        // 在实际数据库中，可能需要使用自增ID或更复杂的ID生成策略
    }

} // namespace minidb