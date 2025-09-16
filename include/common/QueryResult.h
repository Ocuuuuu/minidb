// #pragma once
// #include <string>
// #include <vector>
//
// class QueryResult {
// public:
//     // 存储结果的数据
//     std::vector<std::vector<std::string>> rows; // 每行的记录，使用字符串存储
//
//     // 存储列名
//     std::vector<std::string> columnNames;
//
//     // 查询执行状态
//     bool success;
//     std::string errorMessage; // 错误信息
//
//     // 构造函数
//     QueryResult() : success(true) {}
//
//     // 添加一行结果
//     void addRow(const std::vector<std::string>& row) {
//         rows.push_back(row);
//     }
//
//     // 设置错误信息
//     void setError(const std::string& message) {
//         success = false;
//         errorMessage = message;
//     }
//
//     // 获取列数
//     size_t columnCount() const {
//         return columnNames.size();
//     }
//
//     // 获取行数
//     size_t rowCount() const {
//         return rows.size();
//     }
//
//     // 获取结果
//     const std::vector<std::vector<std::string>>& getRows() const {
//         return rows;
//     }
// };

#ifndef QUERY_RESULT_H
#define QUERY_RESULT_H
#include <vector>
#include <string>
#include <optional>
#include <sstream>
#include <iostream>

namespace minidb {

class QueryResult {
public:
    // 存储查询结果的结构：每行结果为一个向量
    using Row = std::vector<std::string>;

    // 添加一行结果
    void addRow(const Row& row) {
        results_.emplace_back(row);
    }

    // 获取指定行的结果
    const Row& getRow(size_t index) const {
        if (index < results_.size()) {
            return results_[index];
        }
        throw std::out_of_range("Row index out of range");
    }

    // 获取行数
    size_t rowCount() const {
        return results_.size();
    }

    // 获取列数
    size_t columnCount() const {
        return results_.empty() ? 0 : results_[0].size();
    }

    // 根据行列索引获取特定值
    std::optional<std::string> getValue(size_t row, size_t col) const {
        if (row < results_.size() && col < results_[row].size()) {
            return results_[row][col];
        }
        return std::nullopt; // 如果行列索引超出范围，返回空
    }

    // 打印结果到标准输出（调试接口）
    void print() const {
        for (const auto& row : results_) {
            for (size_t j = 0; j < row.size(); ++j) {
                std::cout << row[j];
                if (j < row.size() - 1) {
                    std::cout << ", "; // 用逗号分隔列
                }
            }
            std::cout << std::endl; // 每行结束换行
        }
    }

    // 将结果格式化为JSON字符串
    std::string toJsonString() const {
        std::ostringstream json;
        json << "[\n";

        for (size_t i = 0; i < results_.size(); ++i) {
            json << "  { ";
            const auto& row = results_[i];
            for (size_t j = 0; j < row.size(); ++j) {
                json << "\"" << j << "\": \"" << row[j] << "\"";
                if (j < row.size() - 1) {
                    json << ", ";
                }
            }
            json << " }";
            if (i < results_.size() - 1) {
                json << ",";
            }
            json << "\n";
        }

        json << "]\n";
        return json.str();
    }

private:
    std::vector<Row> results_; // 存储查询结果的二维向量：行数 × 列数
};

} // namespace minidb
#endif // QUERY_RESULT_H