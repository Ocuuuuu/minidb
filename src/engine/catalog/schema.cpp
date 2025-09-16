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
    Schema::Schema(const std::vector<MyColumn>& columns) : columns_(columns) {
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


    const MyColumn& Schema::get_column(uint32_t column_index) const {
        // 检查索引是否在有效范围内
        if (column_index >= columns_.size()) {
            // 索引越界，抛出异常
            throw std::out_of_range("Column index out of range");
        }

        // 返回对应列的常量引用
        return columns_[column_index];
    }

    /**
     * @brief 根据索引获取特定列(int版本)
     *
     * 提供对模式中列的只读访问，通过int类型的索引快速定位特定列。
     * 主要用于解决函数调用不明确的问题。
     *
     * @param column_index 列索引（从0开始）
     * @return const Column& 对应列的常量引用
     * @throw std::out_of_range 如果索引超出有效范围
     */
    // const MyColumn& Schema::get_column(int column_index) const {
    //     // 检查索引是否在有效范围内
    //     if (column_index < 0 || static_cast<uint32_t>(column_index) >= columns_.size()) {
    //         // 索引越界，抛出异常
    //         throw std::out_of_range("Column index out of range");
    //     }
    //
    //     // 返回对应列的常量引用
    //     return columns_[static_cast<uint32_t>(column_index)];
    // }

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

    // 新增方法实现

    /**
     * @brief 检查指定列名是否存在于模式中
     *
     * 该方法提供了一种安全的方式来验证列是否存在，不会抛出异常。
     * 适用于在执行操作前先检查列是否存在的场景。
     *
     * @param column_name 要检查的列名（大小写敏感）
     * @return true-列存在，false-列不存在
     *
     * @example
     * if (schema.has_column("username")) {
     *     // 安全地操作username列
     * }
     */
    bool Schema::has_column(const std::string& column_name) const {
        for (const auto& column : columns_) {
            if (column.name == column_name) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 根据列名获取列的常量引用
     *
     * 通过列名查找对应的列，返回常量引用确保列信息不会被修改。
     * 适用于只读操作，如查询列的类型、长度等元信息。
     *
     * @param column_name 要查找的列名（大小写敏感）
     * @return const Column& 对应列的常量引用
     * @throw std::runtime_error 如果列名不存在于模式中
     *
     * @example
     * const Column& id_col = schema.get_column("id");
     * TypeId id_type = id_col.type; // 安全地读取类型信息
     */
    const MyColumn& Schema::get_column(const std::string& column_name) const {
        for (const auto& column : columns_) {
            if (column.name == column_name) {
                return column;
            }
        }
        throw std::runtime_error("Column not found: " + column_name);
    }

    /**
     * @brief 根据列名获取列的可修改引用
     *
     * 通过列名查找对应的列，返回可修改引用允许修改列信息。
     * 使用时需谨慎，仅应在确实需要修改列元数据的场景下使用。
     *
     * @param column_name 要查找的列名（大小写敏感）
     * @return Column& 对应列的引用
     * @throw std::runtime_error 如果列名不存在于模式中
     *
     * @note 修改列信息可能会影响模式的一致性，建议在受控环境下使用
     * @warning 修改列的偏移量或长度可能会破坏记录布局，需要重新计算模式长度
     */
    MyColumn& Schema::get_column(const std::string& column_name) {
        for (auto& column : columns_) {
            if (column.name == column_name) {
                return column;
            }
        }
        throw std::runtime_error("Column not found: " + column_name);
    }

    /**
     * @brief 获取模式中所有列的名称列表
     *
     * 返回包含所有列名的向量，便于快速获取表的列结构概览。
     * 常用于显示表结构、生成SQL语句或进行列名批量处理。
     *
     * @return std::vector<std::string> 按定义顺序排列的列名向量
     *
     * @example
     * std::vector<std::string> col_names = schema.get_column_names();
     * for (const auto& name : col_names) {
     *     std::cout << "Column: " << name << std::endl;
     * }
     *
     * @note 返回的列名顺序与列在模式中的定义顺序一致
     */
    std::vector<std::string> Schema::get_column_names() const {
        std::vector<std::string> names;
        names.reserve(columns_.size()); // 预分配空间提高效率

        for (const auto& column : columns_) {
            names.push_back(column.name);
        }
        return names;
    }

} // namespace minidb