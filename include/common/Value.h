#pragma once

#include "Types.h"
#include "Exception.h"
#include <variant>
#include <string>
#include <ostream>

namespace minidb {

    class Value {
    public:
        // 构造函数
        Value(); // 默认构造，创建INVALID类型的值
        explicit Value(bool value);
        explicit Value(int32_t value);
        explicit Value(const std::string& value);
        explicit Value(const char* value); // 方便字符串字面量

        // 禁止拷贝和赋值（根据需要可以放开）
        Value(const Value&) = default;
        Value& operator=(const Value&) = default;

        // 类型信息
        TypeId getType() const { return type_id_; }
        bool isNull() const { return type_id_ == TypeId::INVALID; }

        // 类型安全的值获取
        int32_t getAsInt() const;
        bool getAsBool() const;
        std::string getAsString() const;

        // 比较操作
        bool equals(const Value& other) const;
        bool lessThan(const Value& other) const;
        bool greaterThan(const Value& other) const;
        bool lessThanOrEquals(const Value& other) const; // 新增
        bool greaterThanOrEquals(const Value& other) const; // 新增

        // 算术操作（简易版）
        Value add(const Value& other) const;
        Value subtract(const Value& other) const;

        // 转换为字符串表示
        std::string toString() const;

        // 获取值的存储大小（估算）
        size_t getSize() const;

        // 友元函数：输出操作符
        friend std::ostream& operator<<(std::ostream& os, const Value& value);

        // 新增运算符
        bool operator>=(const Value& other) const {
            return !lessThan(other);
        }

        bool operator<=(const Value& other) const {
            return !greaterThan(other);
        }

    private:
        TypeId type_id_{TypeId::INVALID};
        std::variant<bool, int32_t, std::string> value_;
    };

    // 比较操作符重载
    bool operator==(const Value& lhs, const Value& rhs);
    bool operator!=(const Value& lhs, const Value& rhs);
    bool operator<(const Value& lhs, const Value& rhs);
    bool operator>(const Value& lhs, const Value& rhs);

} // namespace minidb