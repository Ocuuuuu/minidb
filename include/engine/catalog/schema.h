//
// Created by 罗飞燕 on 2025/9/10.
//
//定义表的结构模式
#pragma once

#include <vector>
#include <string>
#include "column.h"

namespace minidb {

    /**
     * @brief 表模式定义
     *
     * 表示数据库表的结构定义，包含所有列的集合以及记录的总长度。
     * 相当于表的"蓝图"，描述了如何解析和构建表中的记录。
     */
    class Schema {
    public:
        Schema() = default; ///< 默认构造函数

        /**
         * @brief 构造函数：根据列定义构建模式
         * @param columns 列的集合
         */
        explicit Schema(const std::vector<Column>& columns);

        // 核心查询接口
        const std::vector<Column>& get_columns() const { return columns_; } ///< 获取所有列
        const Column& get_column(uint32_t column_index) const; ///< 根据索引获取特定列
        uint32_t get_column_count() const { return columns_.size(); } ///< 获取列数量
        uint32_t get_length() const { return length_; } ///< 获取一条记录的总长度（字节）

        // 工具方法
        uint32_t get_column_index(const std::string& column_name) const; ///< 根据列名获取列索引

    private:
        std::vector<Column> columns_; ///< 所有列的集合
        uint32_t length_{0};         ///< 一条完整记录的总长度（所有列长度之和）
    };

} // namespace minidb