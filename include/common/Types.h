//
//基本类型定义
//
// include/common/Types.h
#pragma once
#include <string>
#include <variant>
#include <cstdint>

namespace minidb {

    // 支持的数据库数据类型
    enum class DataType {
        INT,
        FLOAT,
        STRING,
        BOOL,
        UNKNOWN
    };

    // 数据库值类
    class Value {
    public:
        std::variant<int32_t, float, std::string, bool> data;
        DataType type;

        Value() : type(DataType::UNKNOWN) {}
        Value(int32_t v) : data(v), type(DataType::INT) {}
        Value(float v) : data(v), type(DataType::FLOAT) {}
        Value(const std::string& v) : data(v), type(DataType::STRING) {}
        Value(bool v) : data(v), type(DataType::BOOL) {}

        // 类型检查和转换方法
        bool is_int() const { return type == DataType::INT; }
        bool is_float() const { return type == DataType::FLOAT; }
        bool is_string() const { return type == DataType::STRING; }
        bool is_bool() const { return type == DataType::BOOL; }

        int32_t as_int() const { return std::get<int32_t>(data); }
        float as_float() const { return std::get<float>(data); }
        std::string as_string() const { return std::get<std::string>(data); }
        bool as_bool() const { return std::get<bool>(data); }
    };

} // namespace minidb