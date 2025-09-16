//定义一列的元数据
#pragma once

#include <string>
#include<stdexcept>
#include "../include/common/Types.h"

namespace minidb{

    /**
     * 列元数据定义
     * 表示数据库表中一个列的结构定义，包含列名、数据类型、长度和在记录中的偏移量。
     */
    struct MyColumn {
        std::string name;   //列的名称，如 "id", "name", "age"
        TypeId type;        // 列的数据类型，如 INTEGER, VARCHAR, BOOLEAN
        uint32_t length;    // 列的长度（字节数），如 INTEGER=4, VARCHAR=20
        uint32_t offset;    // 该列在记录数据中的起始偏移位置（字节）


        MyColumn(const std::string& col_name, TypeId col_type,uint32_t col_length, uint32_t col_offset);

        //修改列名
        void set_name(const std::string& new_name);

        //修改列类型
        void set_type(TypeId new_type);

        //修改列长度
        void set_length(uint32_t new_length);
    };

} // namespace minidb