#include "../include/engine/catalog/schema.h"
#include <stdexcept> // 用于异常处理

namespace minidb {

    /**
     * @brief Schema构造函数实现
     *
     * 根据提供的列定义构建模式，并自动计算每个列的偏移量和记录总长度。
     * 偏移量计算规则：每个列紧接在前一个列之后，连续存储。
     *
     * @param columns 列定义向量，包含表中所有列的信息
     */
    Schema::Schema(const std::vector<Column>& columns) : columns_(columns) {
        // 初始化当前偏移量为0，表示记录的起始位置
        uint32_t current_offset = 0;

        // 遍历所有列，计算每个列的偏移量
        for (auto& column : columns_) {
            // 设置当前列的偏移量（在记录中的起始位置）
            column.offset = current_offset;

            // 更新偏移量：移动到下一个列的起始位置
            // 当前偏移量 + 当前列的长度 = 下一个列的起始位置
            current_offset += column.length;
        }

        // 设置记录的总长度，等于最后一个列的结束位置
        // 也就是所有列长度的总和
        length_ = current_offset;
    }

    /**
     * @brief 根据索引获取特定列
     *
     * 提供对模式中列的只读访问，通过索引快速定位特定列。
     *
     * @param column_index 列索引（从0开始）
     * @return const Column& 对应列的常量引用
     * @throw std::out_of_range 如果索引超出有效范围
     */
    const Column& Schema::get_column(uint32_t column_index) const {
        // 检查索引是否在有效范围内
        if (column_index >= columns_.size()) {
            // 索引越界，抛出异常
            throw std::out_of_range("Column index out of range");
        }

        // 返回对应列的常量引用
        return columns_[column_index];
    }

    /**
     * @brief 根据列名获取列索引
     *
     * 通过列名查找对应的列索引，用于在不知道索引位置时定位列。
     * 使用线性搜索方式查找，适合列数量不多的情况。
     *
     * @param column_name 要查找的列名（大小写敏感）
     * @return uint32_t 列的索引位置（从0开始）
     * @throw std::runtime_error 如果列名不存在于模式中
     */
    uint32_t Schema::get_column_index(const std::string& column_name) const {
        // 遍历所有列，查找匹配的列名
        for (uint32_t i = 0; i < columns_.size(); ++i) {
            // 比较当前列名与目标列名
            if (columns_[i].name == column_name) {
                // 找到匹配的列，返回其索引
                return i;
            }
        }

        // 遍历完所有列仍未找到，抛出异常
        throw std::runtime_error("Column not found: " + column_name);
    }

} // namespace minidb