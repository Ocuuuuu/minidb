#pragma once
#include <string>
#include <vector>

class QueryResult {
public:
    // 存储结果的数据
    std::vector<std::vector<std::string>> rows; // 每行的记录，使用字符串存储
  
    // 存储列名
    std::vector<std::string> columnNames; 

    // 查询执行状态
    bool success; 
    std::string errorMessage; // 错误信息

    // 构造函数
    QueryResult() : success(true) {}

    // 添加一行结果
    void addRow(const std::vector<std::string>& row) {
        rows.push_back(row);
    }

    // 设置错误信息
    void setError(const std::string& message) {
        success = false;
        errorMessage = message;
    }

    // 获取列数
    size_t columnCount() const {
        return columnNames.size();
    }

    // 获取行数
    size_t rowCount() const {
        return rows.size();
    }

    // 获取结果
    const std::vector<std::vector<std::string>>& getRows() const {
        return rows;
    }
};