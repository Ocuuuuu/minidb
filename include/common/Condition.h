//
// Created by lenovo on 2025/9/15.
//

#ifndef MINIDB_CONDITION_H
#define MINIDB_CONDITION_H
#include <string>
#include <variant>

namespace minidb {

    enum class ConditionType {
        EQUALS,    // 表示等于条件
        NOT_EQUALS, // 表示不等于条件
        GREATER_THAN, // 表示大于条件
        LESS_THAN, // 表示小于条件
        // 可以添加更多的条件类型
    };

    class Condition {
    public:
        Condition(const std::string& column_name,
                  const std::variant<int32_t, bool, std::string>& value,
                  ConditionType condition_type)
            : columnName(column_name), value(value), conditionType(condition_type) {}

        const std::string& getColumnName() const { return columnName; }
        const std::variant<int32_t, bool, std::string>& getValue() const { return value; }
        ConditionType getConditionType() const { return conditionType; }

    private:
        std::string columnName; // 要比较的列名
        std::variant<int32_t, bool, std::string> value; // 列值
        ConditionType conditionType; // 条件类型
    };

} // namespace minidb

#endif //MINIDB_CONDITION_H