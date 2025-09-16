//定义表的结构模式
#pragma once

#include <vector>
#include <string>
#include "MyColumn.h"

namespace minidb {

    //表模式定义
    //表示数据库表的结构定义，包含所有列的集合以及记录的总长度。
    class Schema {
    public:
        Schema() = default; //默认构造函数

        explicit Schema(const std::vector<MyColumn>& columns);

        void addColumn(const MyColumn& column) {
            columns_.push_back(column);
            length_ += column.length; // 更新记录长度
        }

        // 核心查询接口
        const std::vector<MyColumn>& get_columns() const { return columns_; } ///< 获取所有列
        const MyColumn& get_column(uint32_t column_index) const; ///< 根据索引获取特定列
        const MyColumn& get_column(int column_index) const; ///< 根据索引获取特定列(int版本)
        uint32_t get_column_count() const { return columns_.size(); } ///< 获取列数量
        uint32_t get_length() const { return length_; } ///< 获取一条记录的总长度（字节）

        // 工具方法
        uint32_t get_column_index(const std::string& column_name) const; ///< 根据列名获取列索引

        //用于AST执行器集成
        bool has_column(const std::string& column_name) const;

        //根据列名获取列（常量版本）
        const MyColumn& get_column(const std::string& column_name) const;

        //根据列名获取列（非常量版本）
        MyColumn& get_column(const std::string& column_name);

        //获取所有列名的列表
        std::vector<std::string> get_column_names() const;

    private:
        std::vector<MyColumn> columns_; // 所有列的集合
        uint32_t length_{0};         //一条完整记录的总长度（所有列长度之和）

        void calculateLength() {
            length_ = 0;
            for (const auto& col : columns_) {
                length_ += col.length;
            }
        }

    };

} // namespace minidb