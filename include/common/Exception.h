#pragma once

#include <stdexcept>
#include <string>

namespace minidb {

    // 基础数据库异常类
    class DatabaseException : public std::runtime_error {
    public:
        explicit DatabaseException(const std::string& msg)
            : std::runtime_error("[Database] " + msg) {}
    };

    // 类型不匹配异常
    class TypeMismatchException : public DatabaseException {
    public:
        explicit TypeMismatchException(const std::string& msg)
            : DatabaseException("Type mismatch: " + msg) {}
    };

    // 超出范围异常
    class OutOfRangeException : public DatabaseException {
    public:
        explicit OutOfRangeException(const std::string& msg)
            : DatabaseException("Out of range: " + msg) {}
    };

    // 空值异常
    class NullValueException : public DatabaseException {
    public:
        explicit NullValueException(const std::string& msg)
            : DatabaseException("Null value: " + msg) {}
    };


} // namespace minidb