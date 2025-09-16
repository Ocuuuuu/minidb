//定义一列的元数据
#pragma once

#include <string>
#include<stdexcept>
#include "../include/common/Types.h"

namespace minidb{

    /**
     * @brief 列元数据定义
     *
     * 表示数据库表中一个列的结构定义，包含列名、数据类型、长度和在记录中的偏移量。
     * 这是模式(Schema)的基本构建块。
     */
    struct MyColumn {
        std::string name;   ///< 列的名称，如 "id", "name", "age"
        TypeId type;        ///< 列的数据类型，如 INTEGER, VARCHAR, BOOLEAN
        uint32_t length;    ///< 列的长度（字节数），如 INTEGER=4, VARCHAR=20
        uint32_t offset;    ///< 该列在记录数据中的起始偏移位置（字节）

        /**
         * @brief 构造函数
         * @param col_name 列名
         * @param col_type 数据类型
         * @param col_length 数据长度
         * @param col_offset 在记录中的偏移量（默认为0，通常由Schema计算）
         */
        MyColumn(const std::string& col_name, TypeId col_type,uint32_t col_length, uint32_t col_offset);

        /**
         * @brief 修改列名（预留接口，当前未实现）
         * @param new_name 新的列名
         * @throw std::runtime_error 当前版本不支持此操作
         */
        void set_name(const std::string& new_name);

        /**
         * @brief 修改列类型（预留接口，当前未实现）
         * @param new_type 新的类型
         * @throw std::runtime_error 当前版本不支持此操作
         */
        void set_type(TypeId new_type);

        /**
         * @brief 修改列长度（预留接口，当前未实现）
         * @param new_length 新的长度
         * @throw std::runtime_error 当前版本不支持此操作
         */
        void set_length(uint32_t new_length);
    };

} // namespace minidb