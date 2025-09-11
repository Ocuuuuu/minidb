//
// Created by 罗飞燕 on 2025/9/10.
//
#include "../include/engine/catalog/column.h"
#include <stdexcept> //抛出异常

namespace minidb {

    /**
 * @brief Column构造函数实现
 *
 * 初始化列的元数据信息。偏移量通常由Schema类在构建时统一计算。
 */
Column::Column(const std::string& col_name, TypeId col_type,
               uint32_t col_length, uint32_t col_offset)
    : name(col_name), type(col_type), length(col_length), offset(col_offset) {

    // 1. 验证列名
    if (name.empty()) {
        throw std::invalid_argument("Column name cannot be empty");
    }

    // 检查列名是否包含非法字符（如空格）
    if (name.find(' ') != std::string::npos) {
        throw std::invalid_argument("Column name cannot contain spaces: " + name);
    }

    // 2. 验证类型
    if (type == TypeId::INVALID) {
        throw std::invalid_argument("Invalid column type for column: " + name);
    }

    // 3. 验证长度
    if (length == 0) {
        throw std::invalid_argument("Column length must be greater than 0 for column: " + name);
    }

    // 4. 类型特定的验证
    switch (type) {
        case TypeId::BOOLEAN:
            if (length != 1) {
                throw std::invalid_argument("BOOLEAN type must have length 1 for column: " + name);
            }
            break;

        case TypeId::INTEGER:
            if (length != 1 && length != 2 && length != 4) {
                throw std::invalid_argument(
                    "INTEGER length must be 1, 2, or 4 bytes for column: " + name +
                    ", got " + std::to_string(length)
                );
            }
            break;

        case TypeId::VARCHAR:
            if (length < 1) {
                throw std::invalid_argument("VARCHAR length must be at least 1 for column: " + name);
            }
            if (length > 65535) {
                throw std::invalid_argument("VARCHAR length too large (max 65535) for column: " + name);
            }
            break;

        default:
            throw std::invalid_argument("Unsupported column type for column: " + name);
    }

    // 5. 验证偏移量
    if (col_offset > 0xFFFF) {
        throw std::invalid_argument("Column offset out of range for column: " + name);
    }
}

    // 预留接口：目前抛出异常，未来可以实现
void Column::set_name(const std::string& new_name) {
    throw std::runtime_error("Column modification is not supported in current version");
    // name = new_name; // 未来取消注释即可实现
}

void Column::set_type(TypeId new_type) {
    throw std::runtime_error("Column modification is not supported in current version");
    // type = new_type;
}

void Column::set_length(uint32_t new_length) {
    throw std::runtime_error("Column modification is not supported in current version");
    // length = new_length;
}

} // namespace minidb