#include "./common/Value.h"
#include "./common/Types.h"
#include <sstream>
#include <stdexcept>

namespace minidb {

// 获取类型名称的辅助函数
const char* getTypeName(TypeId type) {
    switch (type) {
        case TypeId::INVALID: return "INVALID";
        case TypeId::BOOLEAN: return "BOOLEAN";
        case TypeId::INTEGER: return "INTEGER";
        case TypeId::VARCHAR: return "VARCHAR";
        default: return "UNKNOWN";
    }
}

// Value 类的实现
Value::Value() : type_id_(TypeId::INVALID), value_(false) {}

Value::Value(bool value) : type_id_(TypeId::BOOLEAN), value_(value) {}

Value::Value(int32_t value) : type_id_(TypeId::INTEGER), value_(value) {}

Value::Value(const std::string& value) : type_id_(TypeId::VARCHAR), value_(value) {}

Value::Value(const char* value) : type_id_(TypeId::VARCHAR), value_(std::string(value)) {}

int32_t Value::getAsInt() const {
    if (type_id_ != TypeId::INTEGER) {
        throw TypeMismatchException("Expected INTEGER, got " + std::string(getTypeName(type_id_)));
    }
    return std::get<int32_t>(value_);
}

bool Value::getAsBool() const {
    if (type_id_ != TypeId::BOOLEAN) {
        throw TypeMismatchException("Expected BOOLEAN, got " + std::string(getTypeName(type_id_)));
    }
    return std::get<bool>(value_);
}

std::string Value::getAsString() const {
    if (type_id_ != TypeId::VARCHAR) {
        throw TypeMismatchException("Expected VARCHAR, got " + std::string(getTypeName(type_id_)));
    }
    return std::get<std::string>(value_);
}

std::string Value::toString() const {
    if (isNull()) return "NULL";
    
    switch (type_id_) {
        case TypeId::BOOLEAN:
            return std::get<bool>(value_) ? "true" : "false";
        case TypeId::INTEGER:
            return std::to_string(std::get<int32_t>(value_));
        case TypeId::VARCHAR:
            return "'" + std::get<std::string>(value_) + "'";
        default:
            return "UNKNOWN";
    }
}

size_t Value::getSize() const {
    switch (type_id_) {
        case TypeId::BOOLEAN: return sizeof(bool);
        case TypeId::INTEGER: return sizeof(int32_t);
        case TypeId::VARCHAR: return std::get<std::string>(value_).size();
        default: return 0;
    }
}

// 比较操作符实现
bool operator==(const Value& lhs, const Value& rhs) {
    return lhs.equals(rhs);
}

bool Value::equals(const Value& other) const {
    if (type_id_ != other.type_id_) return false;
    if (isNull() && other.isNull()) return true;
    if (isNull() || other.isNull()) return false;
    
    switch (type_id_) {
        case TypeId::BOOLEAN: return std::get<bool>(value_) == std::get<bool>(other.value_);
        case TypeId::INTEGER: return std::get<int32_t>(value_) == std::get<int32_t>(other.value_);
        case TypeId::VARCHAR: return std::get<std::string>(value_) == std::get<std::string>(other.value_);
        default: return false;
    }
}

// 输出操作符
std::ostream& operator<<(std::ostream& os, const Value& value) {
    os << value.toString();
    return os;
}

} // namespace minidb