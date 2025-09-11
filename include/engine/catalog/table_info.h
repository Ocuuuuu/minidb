//
// Created by 罗飞燕 on 2025/9/10.
//
//一张表的完整元数据
#pragma once

#include <string>
#include "schema.h"

namespace minidb {

    /**
     * @brief 表信息类
     *
     * 封装了一张表的完整元信息，包括表名和表结构模式。
     * 这是目录管理中的基本存储单元。
     */
    class TableInfo {
    public:
        /**
         * @brief 构造函数
         * @param table_name 表名
         * @param schema 表的结构模式
         */
        TableInfo(const std::string& table_name, const Schema& schema);

        // 查询接口
        const std::string& get_table_name() const { return table_name_; } ///< 获取表名
        const Schema& get_schema() const { return schema_; } ///< 获取表结构模式
        uint32_t get_table_id() const { return table_id_; } ///< 获取表ID（未来用于关联其他信息）

    private:
        std::string table_name_; ///< 表的名称
        Schema schema_;         ///< 表的结构定义
        uint32_t table_id_;     ///< 表的唯一标识符（可用于未来扩展，如外键关联）
    };

} // namespace minidb