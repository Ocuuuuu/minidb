// // //
// // // Created by lenovo on 2025/9/15.
// // //
// //
// // #ifndef MINIDB_CONDITION_H
// // #define MINIDB_CONDITION_H
// // #include <string>
// // #include <variant>
// //
// // namespace minidb {
// //
// //     enum class ConditionType {
// //         EQUALS,    // 表示等于条件
// //         NOT_EQUALS, // 表示不等于条件
// //         GREATER_THAN, // 表示大于条件
// //         LESS_THAN, // 表示小于条件
// //         // 可以添加更多的条件类型
// //     };
// //
// //     class Condition {
// //     public:
// //         Condition(const std::string& column_name,
// //                   const std::variant<int32_t, bool, std::string>& value,
// //                   ConditionType condition_type)
// //             : columnName(column_name), value(value), conditionType(condition_type) {}
// //
// //         const std::string& getColumnName() const { return columnName; }
// //         const std::variant<int32_t, bool, std::string>& getValue() const { return value; }
// //         ConditionType getConditionType() const { return conditionType; }
// //
// //     private:
// //         std::string columnName; // 要比较的列名
// //         std::variant<int32_t, bool, std::string> value; // 列值
// //         ConditionType conditionType; // 条件类型
// //     };
// //
// // } // namespace minidb
// //
// // #endif //MINIDB_CONDITION_H
//
// #pragma once
//
// #include <string>
// #include <variant>
// #include "Value.h" // 确保包含 Value 的定义
//
// namespace minidb {
//
//     enum class ConditionOperator {
//         EQUALS,
//         NOT_EQUALS,
//         GREATER_THAN,
//         LESS_THAN,
//         // 其他条件类型...
//     };
//
//     class Condition {
//     public:
//         Condition(const std::string& column, ConditionOperator op, const Value& value)
//             : column_(column), operator_(op), value_(value) {}
//
//         const std::string& getColumnName() const {
//             return column_;
//         }
//
//         ConditionOperator getOperator() const {
//             return operator_;
//         }
//
//         const Value& getValue() const {
//             return value_;
//         }
//
//         bool evaluate(const Value& column_value) const {
//             switch (operator_) {
//                 case ConditionOperator::EQUALS:
//                     return column_value.equals(value_);
//                 case ConditionOperator::NOT_EQUALS:
//                     return !column_value.equals(value_);
//                 case ConditionOperator::GREATER_THAN:
//                     return column_value.greaterThan(value_);
//                 case ConditionOperator::LESS_THAN:
//                     return column_value.lessThan(value_);
//                     // 可以添加其他条件的实现
//                 default:
//                     throw std::runtime_error("Unsupported condition operator");
//             }
//         }
//
//     private:
//         std::string column_; // 列名
//         ConditionOperator operator_; // 操作符
//         Value value_; // 用于比较的值
//     };
//
// } // namespace minid // namespace minidb