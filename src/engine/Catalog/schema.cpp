//
// Created by 罗飞燕 on 2025/9/10.
//
#include "../include/engine/catalog/schema.h"
#include <stdexcept> // 用于异常处理

namespace minidb {

    /**
     * @brief Schema构造函数实现
     *
     * 根据提供的列定义构建模式，并自动计算每个列的偏移量和记录总长度。
     * 偏移量计算规则：每个列紧接在前一个列之后。
     */
    //需要再次确定
    Schema::Schema(const std::vector<Column>& columns) : columns_(columns) {
        uint32_t current_offset = 0;
        for (auto& column : columns_) {
            // 为每个列设置正确的偏移量
            column.offset = current_offset;
            // 移动到下一个列的起始位置
            current_offset += column.length;
        }
        length_ = current_offset; // 记录总长度等于最后一个列的结束位置
    }

    /**
     * @brief 根据索引获取特定列
     * @param column_index 列索引（从0开始）
     * @return 对应列的常量引用
     * @throw std::out_of_range 如果索引超出范围
     */
    //具体实现方法待定
    const Column& Schema::get_column(uint32_t column_index) const {
        if (column_index >= columns_.size()) {
            throw std::out_of_range("Column index out of range");
        }
        return columns_[column_index];
    }

    /**
     * @brief 根据列名获取列索引
     * @param column_name 要查找的列名
     * @return 列的索引位置
     * @throw std::runtime_error 如果列名不存在
     */
    //具体实现方法待定
    uint32_t Schema::get_column_index(const std::string& column_name) const {
        for (uint32_t i = 0; i < columns_.size(); ++i) {
            if (columns_[i].name == column_name) {
                return i;
            }
        }
        throw std::runtime_error("Column not found: " + column_name);
    }

} // namespace minidb