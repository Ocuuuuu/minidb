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
    using Row = std::vector<std::string>;

    QueryResult() = default;

    void setColumnNames(const std::vector<std::string>& names) {
        columnNames_ = names;
    }

    const std::vector<std::string>& getColumnNames() const {
        return columnNames_;
    }

    // 添加一行结果
    void addRow(const Row& row) {
        results_.push_back(row);
    }

    // 获取指定行
    const Row& getRow(size_t index) const {
        if (index < results_.size()) {
            return results_[index];
        }
        throw std::out_of_range("Row index out of range");
    }

    // 行数/列数
    size_t rowCount() const { return results_.size(); }
    size_t columnCount() const { return results_.empty() ? 0 : results_[0].size(); }

    // 获取具体单元格
    std::optional<std::string> getValue(size_t row, size_t col) const {
        if (row < results_.size() && col < results_[row].size()) {
            return results_[row][col];
        }
        return std::nullopt;
    }

    // 打印（调试使用）
    void print() const {
        if (results_.empty()) {
            std::cout << "(empty)" << std::endl;
            return;
        }
        for (const auto& row : results_) {
            for (size_t i = 0; i < row.size(); ++i) {
                std::cout << row[i] << (i + 1 < row.size() ? "\t" : "\n");
            }
        }
    }

    // 转成 JSON 字符串
    std::string toJsonString() const {
        std::ostringstream json;
        json << "[\n";
        for (size_t i = 0; i < results_.size(); ++i) {
            json << "  { ";
            const auto& row = results_[i];
            for (size_t j = 0; j < row.size(); ++j) {
                json << "\"" << j << "\": \"" << row[j] << "\"";
                if (j < row.size() - 1) json << ", ";
            }
            json << " }";
            if (i < results_.size() - 1) json << ",";
            json << "\n";
        }
        json << "]";
        return json.str();
    }

private:
    std::vector<std::string> columnNames_;
    std::vector<Row> results_; // 二维数组存所有行列
};

} // namespace minidb

#endif // QUERY_RESULT_H