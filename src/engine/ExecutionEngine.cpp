// #include "../../include/engine/ExecutionEngine.h"
// #include <cstring>
// #include "../../include/engine/catalog/schema.h"
// #include "../../include/engine/catalog/catalog_manager.h"
// #include "../../include/compiler/AST.h"
// #include "storage/Page.h"
// #include "storage/BufferManager.h"
// #include <stdexcept>
//
// #include "common/Condition.h"
// #include "common/Value.h"
//
// namespace minidb {
//
// ExecutionEngine::ExecutionEngine(std::shared_ptr<CatalogManager> catalog,
//                                  std::shared_ptr<storage::BufferManager> bufferManager)
//     : catalog_(std::move(catalog)), bufferManager_(std::move(bufferManager)) {
// }
//
// // 执行CREATE TABLE操作
// bool ExecutionEngine::executeCreateTable(const CreateTableAST& create_ast) {
//     return catalog_->create_table_from_ast(create_ast);
// }
//
// // 执行INSERT操作
// bool ExecutionEngine::executeInsert(const InsertAST& insert_ast) {
//     const TableInfo* table_info = catalog_->get_table(insert_ast.tableName);
//     if (!table_info) {
//         throw std::runtime_error("Table does not exist: " + insert_ast.tableName);
//     }
//
//     char record_data[PAGE_SIZE]; // 记录的数据，根据实际schema定义
//     auto& schema = table_info->get_schema();
//
//     // 初始化记录数据，将其清空
//     std::memset(record_data, 0, sizeof(record_data));
//
//     for (size_t i = 0; i < insert_ast.values.size(); ++i) {
//         if (i >= schema.get_column_count()) {  // 更改这个条件，固定无误
//             throw std::runtime_error("Value count does not match column count.");
//         }
//
//         const MyColumn& column = schema.get_column(static_cast<uint32_t>(i)); // 显式转换为uint32_t
//         Value value;
//
//         // 根据列类型转换插入的字符串值
//         if (column.type == TypeId::INTEGER) {
//             value = Value(std::stoi(insert_ast.values[i]));
//         } else if (column.type == TypeId::BOOLEAN) {
//             value = Value(insert_ast.values[i] == "true");
//         } else if (column.type == TypeId::VARCHAR) {
//             value = Value(insert_ast.values[i]);
//         } else {
//             throw std::runtime_error("Unsupported column type for INSERT.");
//         }
//
//         // 将生成的Value数据转换为记录格式并填入record_data中
//         switch (column.type) {
//             case TypeId::INTEGER: {
//                 int32_t int_value = value.getAsInt();
//                 std::memcpy(record_data + column.offset, &int_value, sizeof(int32_t));
//                 break;
//             }
//             case TypeId::BOOLEAN: {
//                 bool bool_value = value.getAsBool();
//                 std::memcpy(record_data + column.offset, &bool_value, sizeof(bool));
//                 break;
//             }
//             case TypeId::VARCHAR: {
//                 std::string str_value = value.getAsString();
//                 size_t str_length = str_value.length();
//                 std::memcpy(record_data + column.offset, &str_length, sizeof(size_t)); // 假设将长度存储在前面
//                 std::memcpy(record_data + column.offset + sizeof(size_t), str_value.c_str(), str_length);
//                 break;
//             }
//             default:
//                 throw std::runtime_error("Unsupported column type for INSERT.");
//         }
//     }
//     // 执行插入到页面
//     RID rid; // 记录ID（如果需要）
//     storage::Page* page = bufferManager_->fetchPage(table_info->getFirstPageID());
//     return page->insertRecord(record_data, sizeof(record_data), &rid); // 将记录插入到页面
// }
//
//
// // 执行SELECT操作
// QueryResult ExecutionEngine::executeSelect(const SelectAST& select_ast) {
//     return selectFromTable(select_ast);
//
// }
// // 执行SELECT操作
// QueryResult ExecutionEngine::selectFromTable(const SelectAST& select_ast) {
//     const TableInfo* table_info = catalog_->get_table(select_ast.tableName);
//     if (!table_info) {
//         throw std::runtime_error("Table does not exist: " + select_ast.tableName);
//     }
//
//     QueryResult result;  // 查询结果
//     const Schema& schema = table_info->get_schema(); // 获取表的模式
//     storage::Page* page = bufferManager_->fetchPage(table_info->getFirstPageID()); // 获取第一页
//
//     RID rid{0, 0}; // 初始化RID
//
//     if (select_ast.columns.size() == 1 && select_ast.columns[0] == "*") {
//         // 查询所有列
//         while (page->getNextRecord(rid)) { // 使用 getNextRecord
//             char buffer[PAGE_SIZE]; // 缓存记录
//             if (page->getRecord(rid, buffer)) {
//                 std::vector<Value> rowValues; // 每行的值
//                 for (uint32_t colIndex = 0; colIndex < schema.get_column_count(); ++colIndex) {
//                     const MyColumn& column = schema.get_column(colIndex);
//                     Value value;
//
//                     // 根据列类型读取值
//                     if (column.type == TypeId::INTEGER) {
//                         value = Value(*reinterpret_cast<int32_t*>(buffer + column.offset));
//                     } else if (column.type == TypeId::BOOLEAN) {
//                         value = Value(*reinterpret_cast<bool*>(buffer + column.offset));
//                     } else if (column.type == TypeId::VARCHAR) {
//                         // 假设以'\0'结束的字符串存储
//                         value = Value(std::string(buffer + column.offset));
//                     }
//                     rowValues.push_back(value); // 将值添加到行数据中
//                 }
//
//                 // 转换 rowValues 为字符串
//                 std::vector<std::string> rowStrings; // 创建一个新的字符串向量
//                 for (const auto& val : rowValues) {
//                     rowStrings.push_back(val.toString()); // 假设Value有toString()方法
//                 }
//                 result.addRow(rowStrings); // 添加字符串行到结果中
//             }
//         }
//     } else {
//         // 查询特定列
//         for (const auto& column_name : select_ast.columns) {
//             if (!schema.has_column(column_name)) {
//                 throw std::runtime_error("Column does not exist: " + column_name);
//             }
//         }
//
//         while (page->getNextRecord(rid)) { // 使用 getNextRecord
//             char buffer[PAGE_SIZE];
//             if (page->getRecord(rid, buffer)) {
//                 std::vector<Value> rowValues; // 读取每行的值
//                 for (const auto& column_name : select_ast.columns) {
//                     uint32_t colIndex = schema.get_column_index(column_name);
//                     const MyColumn& column = schema.get_column(colIndex);
//                     Value value;
//
//                     // 根据列类型读取值
//                     if (column.type == TypeId::INTEGER) {
//                         value = Value(*reinterpret_cast<int32_t*>(buffer + column.offset));
//                     } else if (column.type == TypeId::BOOLEAN) {
//                         value = Value(*reinterpret_cast<bool*>(buffer + column.offset));
//                     } else if (column.type == TypeId::VARCHAR) {
//                         value = Value(std::string(buffer + column.offset)); // 假设以'\0'结束的字符串存储
//                     }
//                     rowValues.push_back(value); // 将读取的值添加到行数据中
//                 }
//                 // 转换 rowValues 为字符串
//                 std::vector<std::string> rowStrings; // 创建一个新的字符串向量
//                 for (const auto& val : rowValues) {
//                     rowStrings.push_back(val.toString()); // 将Value对象转换为字符串
//                 }
//                 result.addRow(rowStrings); // 添加字符串行到结果中
//             }
//         }
//     }
//     return result; // 返回查询结果
// }
//
// // 执行DELETE操作
// // bool ExecutionEngine::executeDelete(const DeleteAST& delete_ast) {
// //     const TableInfo* table_info = catalog_->get_table(delete_ast.tableName);
// //     if (!table_info) {
// //         throw std::runtime_error("Table does not exist: " + delete_ast.tableName);
// //     }
// //
// //     storage::Page* page = bufferManager_->fetchPage(table_info->getFirstPageID());
// //
// //     // 根据条件查找并删除记录
// //     if (delete_ast.condition.has_value()) {
// //         const auto& condition = delete_ast.condition.value();
// //         for (RID rid; page->getNextRecord(rid); ) {
// //             char buffer[PAGE_SIZE];
// //             if (page->getRecord(rid, buffer)) {
// //                 // 从buffer中提取出字段值进行条件判断
// //                 Value value; // 假设我们将要进行比较的值
// //                 // 根据 condition 检查是否满足条件
// //                 // if (value.equals(condition)) {
// //                 //     page->deleteRecord(rid);  // 如果满足条件，则删除该记录
// //                 // }
// //             }
// //         }
// //     } else {
// //         // 无条件，删除所有记录
// //         for (RID rid; page->getNextRecord(rid); ) {
// //             page->deleteRecord(rid);
// //         }
// //     }
// //
// //     return true; // 执行成功与否返回
// // }
//
// // 顺序扫描
//     QueryResult ExecutionEngine::executeSeqScan(const SeqScanAST& seqScanAST) {
//     const TableInfo* table_info = catalog_->get_table(seqScanAST.tableName);
//     if (!table_info) {
//         throw std::runtime_error("Table does not exist: " + seqScanAST.tableName);
//     }
//
//     QueryResult result; // 保存扫描结果
//     const Schema& schema = table_info->get_schema();
//     storage::Page* page = bufferManager_->fetchPage(table_info->getFirstPageID());
//
//     RID rid{0, 0}; // 初始化RID
//     while (page->getNextRecord(rid)) { // 循环获取每个记录
//         char buffer[PAGE_SIZE];
//         if (page->getRecord(rid, buffer)) {
//             std::vector<Value> rowValues;
//             for (uint32_t colIndex = 0; colIndex < schema.get_column_count(); ++colIndex) {
//                 const MyColumn& column = schema.get_column(colIndex);
//                 if (column.type == TypeId::INTEGER) {
//                     rowValues.emplace_back(*reinterpret_cast<int32_t*>(buffer + column.offset));
//                 } else if (column.type == TypeId::BOOLEAN) {
//                     rowValues.emplace_back(*reinterpret_cast<bool*>(buffer + column.offset));
//                 } else if (column.type == TypeId::VARCHAR) {
//                     rowValues.emplace_back(std::string(buffer + column.offset));
//                 }
//             }
//             // 转换为字符串并添加到结果
//             std::vector<std::string> rowStrings;
//             for (const auto& value : rowValues) {
//                 rowStrings.push_back(value.toString());
//             }
//             result.addRow(rowStrings);
//         }
//     }
//     return result; // 返回完整的查询结果
// }
//
// // //过滤
// //     QueryResult ExecutionEngine::executeFilter(const FilterAST& filterAST) {
// //     QueryResult result = executeSeqScan(filterAST.seqScan); // 首先执行顺序扫描
// //
// //     // 过滤处理
// //     QueryResult filteredResult;
// //     for (const auto& row : result.getRows()) {
// //         // 获取 condition 中的列名、值和条件类型
// //         const std::string& columnName = filterAST.condition.getColumnName();
// //         const auto& conditionValue = filterAST.condition.getValue();
// //         ConditionType conditionType = filterAST.condition.getConditionType();
// //
// //         // 在行中查找对应列值
// //         for (size_t i = 0; i < row.size(); ++i) {
// //             // 假设 row[i] 是来自 `Value` 的返回值，需要查找相应列的值
// //             if (schema.get_columns()[i].name == columnName) {
// //                 const Value& rowValue = row[i];
// //                 bool matches = false;
// //
// //                 // 比较条件类型
// //                 switch (conditionType) {
// //                     case ConditionType::EQUALS:
// //                         matches = (rowValue.equals(std::get<decltype(rowValue)>(conditionValue)));
// //                         break;
// //                     case ConditionType::NOT_EQUALS:
// //                         matches = !(rowValue.equals(std::get<decltype(rowValue)>(conditionValue)));
// //                         break;
// //                     case ConditionType::GREATER_THAN:
// //                         matches = rowValue.greaterThan(std::get<decltype(rowValue)>(conditionValue));
// //                         break;
// //                     case ConditionType::LESS_THAN:
// //                         matches = rowValue.lessThan(std::get<decltype(rowValue)>(conditionValue));
// //                         break;
// //                         // 可以添加更多条件类型的逻辑
// //                 }
// //
// //                 // 如果匹配条件，则添加到结果
// //                 if (matches) {
// //                     filteredResult.addRow(row);
// //                 }
// //                 break; // 找到对应列后可以直接跳出
// //             }
// //         }
// //     }
// //     return filteredResult; // 返回过滤后的结果
// // }
//
//
// //投影
//     QueryResult ExecutionEngine::executeProject(const ProjectAST& projectAST) {
//     QueryResult result = executeSeqScan(projectAST.seqScan); // 首先执行顺序扫描
//
//     // 投影处理
//     QueryResult projectedResult;
//     for (const auto& row : result.getRows()) {
//         std::vector<std::string> projectedRow;
//         for (const auto& columnName : projectAST.columns) {
//             // 通过列名查找并添加到投影结果
//             for (size_t i = 0; i < row.size(); ++i) {
//                 if (row[i] == columnName) {
//                     projectedRow.push_back(row[i]); // 假设 row[i] 存储的是列的值
//                     break; // 找到后可以直接跳出
//                 }
//             }
//         }
//         projectedResult.addRow(projectedRow);
//     }
//     return projectedResult; // 返回投影后的结果
// }
//
//
//
// } // namespace minidb
//

#include "../include/engine/ExecutionEngine.h"
#include "engine/catalog/catalog_manager.h"
#include "../include/storage/BufferManager.h"
#include "common/QueryResult.h"
#include "common/Value.h"
#include <cstring>


namespace minidb {

// 执行创建表
QueryResult ExecutionEngine::executeCreateTable(const nlohmann::json& plan) {
    std::string tableName = plan["tableName"];
    Schema schema;

    uint32_t offset = 0; // 偏移量初始化

    // 解析AST中的列定义并构造Schema
    for (const auto& col : plan["columns"]) {
        TypeId type_id = catalog_->getTypeIdFromString(col["type"]);
        uint32_t length = (type_id == TypeId::VARCHAR) ? 255 : 4; // 假设VARCHAR最大长度为255
        MyColumn new_column(col["name"], type_id, length, offset);
        schema.addColumn(new_column);
        offset += length; // 更新偏移量
    }

    if (!catalog_->create_table(tableName, schema)) {
        handleError("Failed to create table: " + tableName);
    }

    return QueryResult(); // 返回成功或空的结果集
}

// 执行插入操作
QueryResult ExecutionEngine::executeInsert(const nlohmann::json& plan) {
    std::string tableName = plan["tableName"];
    std::vector<std::string> values = plan["values"];

    const TableInfo* table_info = catalog_->get_table(tableName);
    if (!table_info) {
        handleError("Table does not exist: " + tableName);
    }

    PageID first_page_id = table_info->getFirstPageID();
    storage::Page* page = bufferManager_->fetchPage(first_page_id);

    char record_data[PAGE_SIZE];
    std::memset(record_data, 0, sizeof(record_data));

    const Schema& schema = table_info->get_schema();
    for (size_t i = 0; i < values.size(); ++i) {
        const MyColumn& column = schema.get_column(static_cast<uint32_t>(i));
        if (column.type == TypeId::INTEGER) {
            int32_t value = std::stoi(values[i]);
            std::memcpy(record_data + column.offset, &value, sizeof(int32_t));
        } else if (column.type == TypeId::BOOLEAN) {
            bool value = (values[i] == "true");
            std::memcpy(record_data + column.offset, &value, sizeof(bool));
        } else if (column.type == TypeId::VARCHAR) {
            std::string value = values[i];
            std::memcpy(record_data + column.offset, value.c_str(), value.size());
            // 可能还需要处理长度
        }
    }

    RID rid;
    if (!page->insertRecord(record_data, sizeof(record_data), &rid)) {
        handleError("Failed to insert record.");
    }

    page->setDirty(true);
    return QueryResult(); // 返回成功或空的结果集
}

// 执行选择操作
QueryResult ExecutionEngine::executeSelect(const nlohmann::json& plan) {
    std::string tableName = plan["tableName"];
    std::vector<std::string> columns = plan["columns"];

    const TableInfo* table_info = catalog_->get_table(tableName);
    if (!table_info) {
        handleError("Table does not exist: " + tableName);
    }

    QueryResult result;
    PageID first_page_id = table_info->getFirstPageID();
    storage::Page* page = bufferManager_->fetchPage(first_page_id);
    RID rid{0, 0};
    char buffer[PAGE_SIZE];

    while (page->getNextRecord(rid)) {
        if (page->getRecord(rid, buffer)) {
            std::vector<std::string> rowValues;
            for (const auto& column : columns) {
                uint32_t colIndex = table_info->get_schema().get_column_index(column);
                const MyColumn& columnInfo = table_info->get_schema().get_column(colIndex);
                if (columnInfo.type == TypeId::INTEGER) {
                    rowValues.push_back(std::to_string(*reinterpret_cast<int32_t*>(buffer + columnInfo.offset)));
                } else if (columnInfo.type == TypeId::BOOLEAN) {
                    rowValues.push_back(*reinterpret_cast<bool*>(buffer + columnInfo.offset) ? "true" : "false");
                } else if (columnInfo.type == TypeId::VARCHAR) {
                    rowValues.push_back(std::string(buffer + columnInfo.offset));
                }
            }
            result.addRow(rowValues);
        }
    }
    return result; // 返回查询结果
}


// 执行删除操作
    // 执行删除操作
    QueryResult ExecutionEngine::executeDelete(const nlohmann::json& plan) {
    std::string tableName = plan["tableName"];
    const TableInfo* table_info = catalog_->get_table(tableName);
    if (!table_info) {
        handleError("Table does not exist: " + tableName);
    }

    PageID first_page_id = table_info->getFirstPageID();
    storage::Page* page = bufferManager_->fetchPage(first_page_id);
    RID rid{0, 0};

    if (plan.contains("condition")) {
        auto condition = plan["condition"];

        // 获取列名和条件值
        std::string column_name = condition["column"];
        std::string condition_value = condition["value"];
        std::string condition_op = condition["op"];

        uint32_t colIndex = table_info->get_schema().get_column_index(column_name);

        while (page->getNextRecord(rid)) {
            char buffer[PAGE_SIZE];
            if (page->getRecord(rid, buffer)) {
                Value column_value;
                const MyColumn& column = table_info->get_schema().get_column(colIndex);

                if (column.type == TypeId::INTEGER) {
                    column_value = Value(*reinterpret_cast<int32_t*>(buffer + column.offset));
                } else if (column.type == TypeId::BOOLEAN) {
                    column_value = Value(*reinterpret_cast<bool*>(buffer + column.offset));
                } else if (column.type == TypeId::VARCHAR) {
                    column_value = Value(std::string(buffer + column.offset));
                }

                // 验证记录是否满足条件
                bool condition_met = false;

                if (condition_op == "EQUALS") {
                    condition_met = column_value.equals(Value(condition_value));
                } else if (condition_op == "NOT_EQUALS") {
                    condition_met = !column_value.equals(Value(condition_value));
                } else if (condition_op == "GREATER_THAN") {
                    condition_met = column_value.greaterThan(Value(condition_value));
                } else if (condition_op == "LESS_THAN") {
                    condition_met = column_value.lessThan(Value(condition_value));
                } else if (condition_op == "GREATER_THAN_OR_EQUAL") {
                    condition_met = column_value.greaterThanOrEquals(Value(condition_value));
                } else if (condition_op == "LESS_THAN_OR_EQUAL") {
                    condition_met = column_value.lessThanOrEquals(Value(condition_value));
                }

                if (condition_met) {
                    page->deleteRecord(rid);
                }
            }
        }
    } else {
        while (page->getNextRecord(rid)) {
            page->deleteRecord(rid);
        }
    }

    page->setDirty(true);
    return QueryResult(); // 返回成功或空的结果集
}


// 主要执行计划函数
QueryResult ExecutionEngine::executePlan(const nlohmann::json& plan) {
    std::string type = plan["type"];

    if (type == "CreateTable") {
        return executeCreateTable(plan);
    } else if (type == "Insert") {
        return executeInsert(plan);
    } else if (type == "Select") {
        return executeSelect(plan);
    } else if (type == "Delete") {
        return executeDelete(plan);
    } else {
        throw std::runtime_error("Unsupported execution plan type: " + type);
    }
}

} // mespace minidbbce minidb