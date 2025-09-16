#include "../include/engine/ExecutionEngine.h"
#include "common/Exception.h"
#include <cstring>
#include <algorithm>

namespace minidb {

void ExecutionEngine::scanTablePages(const TableInfo *table_info,
                                     const std::function<void(storage::Page *, RID &)> &callback) {
    PageID pid = table_info->getFirstPageID();
    while (pid != INVALID_PAGE_ID) {
        storage::Page *page = bufferManager_->fetchPage(pid);
        if (!page) {
            throw std::runtime_error("Failed to fetch page: " + std::to_string(pid));
        }
        RID rid{pid, static_cast<uint16_t>(-1)}; // 初始化为 -1 才能取到第0条
        while (page->getNextRecord(rid)) {
            callback(page, rid);
        }
        pid = page->getNextPageId();
    }
}

PageID ExecutionEngine::appendNewPageToTable(TableInfo *table_info) {
    PageID new_pid = bufferManager_->allocatePage();
    storage::Page *new_page = bufferManager_->fetchPage(new_pid);
    new_page->initAsDataPage();
    new_page->setNextPageId(INVALID_PAGE_ID);

    PageID last_pid = table_info->getFirstPageID();
    storage::Page *last_page = bufferManager_->fetchPage(last_pid);
    while (last_page->getNextPageId() != INVALID_PAGE_ID) {
        last_pid = last_page->getNextPageId();
        last_page = bufferManager_->fetchPage(last_pid);
    }
    last_page->setNextPageId(new_pid);
    last_page->setDirty(true);

    new_page->setDirty(true);
    return new_pid;
}

QueryResult ExecutionEngine::executeCreateTable(const nlohmann::json &plan) {
    std::string tableName = plan["tableName"];
    Schema schema;

    uint32_t offset = 0;
    for (const auto &col : plan["columns"]) {
        TypeId type_id = catalog_->getTypeIdFromString(col["type"]);
        uint32_t length = (type_id == TypeId::VARCHAR) ? 255 : 4;
        schema.addColumn(MyColumn(col["name"], type_id, length, offset));
        offset += length;
    }

    if (!catalog_->create_table(tableName, schema)) {
        handleError("Failed to create table: " + tableName);
    }

    TableInfo *table_info = catalog_->get_table(tableName);
    if (!table_info) {
        handleError("Table creation failed: " + tableName);
    }

    PageID first_page_id = bufferManager_->allocatePage();
    storage::Page *page = bufferManager_->fetchPage(first_page_id);
    page->initAsDataPage();
    page->setNextPageId(INVALID_PAGE_ID);
    page->setDirty(true);

    table_info->setFirstPageID(first_page_id);

    return QueryResult();
}

QueryResult ExecutionEngine::executeInsert(const nlohmann::json &plan) {
    std::string tableName = plan["tableName"];
    std::vector<std::string> values = plan["values"];

    TableInfo *table_info = catalog_->get_table(tableName);
    if (!table_info) handleError("Table does not exist: " + tableName);

    const Schema &schema = table_info->get_schema();
    size_t record_size = 0;
    for (uint32_t i = 0; i < schema.get_column_count(); ++i) {
        const MyColumn &column = schema.get_column(i);
        record_size += (column.type == TypeId::VARCHAR) ? column.length :
                       (column.type == TypeId::BOOLEAN ? sizeof(bool) : sizeof(int32_t));
    }

    std::vector<char> record_data(record_size, 0);
    size_t current_offset = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        const MyColumn &column = schema.get_column(static_cast<uint32_t>(i));
        if (column.type == TypeId::INTEGER) {
            int32_t v = std::stoi(values[i]);
            std::memcpy(record_data.data() + current_offset, &v, sizeof(int32_t));
            current_offset += sizeof(int32_t);
        } else if (column.type == TypeId::BOOLEAN) {
            bool v = (values[i] == "true" || values[i] == "1");
            std::memcpy(record_data.data() + current_offset, &v, sizeof(bool));
            current_offset += sizeof(bool);
        } else if (column.type == TypeId::VARCHAR) {
            size_t copy_size = std::min(values[i].size(), static_cast<size_t>(column.length));
            std::memcpy(record_data.data() + current_offset, values[i].c_str(), copy_size);
            current_offset += column.length;
        }
    }

    PageID pid = table_info->getFirstPageID();
    storage::Page *page = bufferManager_->fetchPage(pid);
    RID rid;
    while (!page->insertRecord(record_data.data(), record_size, &rid)) {
        if (page->getNextPageId() == INVALID_PAGE_ID) {
            pid = appendNewPageToTable(table_info);
            page = bufferManager_->fetchPage(pid);
        } else {
            pid = page->getNextPageId();
            page = bufferManager_->fetchPage(pid);
        }
    }
    page->setDirty(true);

    return QueryResult();
}

// QueryResult ExecutionEngine::executeSelect(const nlohmann::json &plan) {
//     std::string tableName = plan["tableName"];
//     std::vector<std::string> columns = plan["columns"];
//
//     const TableInfo *table_info = catalog_->get_table(tableName);
//     if (!table_info) handleError("Table does not exist: " + tableName);
//
//     QueryResult result;
//     scanTablePages(table_info, [&](storage::Page *page, RID &rid) {
//         char buffer[PAGE_SIZE];
//         if (page->getRecord(rid, buffer)) {
//             std::vector<std::string> rowValues;
//             for (const auto &column : columns) {
//                 uint32_t colIndex = table_info->get_schema().get_column_index(column);
//                 const MyColumn &columnInfo = table_info->get_schema().get_column(colIndex);
//                 if (columnInfo.type == TypeId::INTEGER) {
//                     rowValues.push_back(std::to_string(*reinterpret_cast<int32_t *>(buffer + columnInfo.offset)));
//                 } else if (columnInfo.type == TypeId::BOOLEAN) {
//                     rowValues.push_back(*reinterpret_cast<bool *>(buffer + columnInfo.offset) ? "true" : "false");
//                 } else if (columnInfo.type == TypeId::VARCHAR) {
//                     rowValues.push_back(std::string(buffer + columnInfo.offset));
//                 }
//             }
//             result.addRow(rowValues);
//         }
//     });
//     return result;
// }
//


    // SELECT 执行
    QueryResult ExecutionEngine::executeSelect(const nlohmann::json &plan) {
    std::string tableName = plan["tableName"];
    TableInfo *tableInfo = catalog_->get_table(tableName);
    if (!tableInfo) {
        throw std::runtime_error("Table not found: " + tableName);
    }

    QueryResult result;

    // 设置列名
    std::vector<std::string> colNames;
    for (auto &col : tableInfo->get_schema().get_columns()) {
        colNames.push_back(col.get_name());
    }
    result.setColumnNames(colNames);

    // 仅扫描首个 page
    PageID pid = tableInfo->getFirstPageID();
    scanSinglePage(pid, tableInfo->get_schema(), result);

    return result;
}

QueryResult ExecutionEngine::executeDelete(const nlohmann::json &plan) {
    std::string tableName = plan["tableName"];
    TableInfo *table_info = catalog_->get_table(tableName);
    if (!table_info) handleError("Table does not exist: " + tableName);

    if (plan.contains("condition")) {
        auto condition = plan["condition"];
        std::string column_name = condition["column"];
        std::string condition_value = condition["value"];
        std::string condition_op = condition["op"];
        uint32_t colIndex = table_info->get_schema().get_column_index(column_name);

        scanTablePages(table_info, [&](storage::Page *page, RID &rid) {
            char buffer[PAGE_SIZE];
            if (page->getRecord(rid, buffer)) {
                const MyColumn &column = table_info->get_schema().get_column(colIndex);
                Value col_val;
                if (column.type == TypeId::INTEGER)
                    col_val = Value(*reinterpret_cast<int32_t *>(buffer + column.offset));
                else if (column.type == TypeId::BOOLEAN)
                    col_val = Value(*reinterpret_cast<bool *>(buffer + column.offset));
                else if (column.type == TypeId::VARCHAR)
                    col_val = Value(std::string(buffer + column.offset));

                bool match = false;
                if (condition_op == "EQUALS") match = col_val.equals(Value(condition_value));
                else if (condition_op == "NOT_EQUALS") match = !col_val.equals(Value(condition_value));
                else if (condition_op == "GREATER_THAN") match = col_val.greaterThan(Value(condition_value));
                else if (condition_op == "LESS_THAN") match = col_val.lessThan(Value(condition_value));
                else if (condition_op == "GREATER_THAN_OR_EQUAL") match = col_val.greaterThanOrEquals(Value(condition_value));
                else if (condition_op == "LESS_THAN_OR_EQUAL") match = col_val.lessThanOrEquals(Value(condition_value));

                if (match) {
                    page->deleteRecord(rid);
                    page->setDirty(true);
                }
            }
        });
    } else {
        scanTablePages(table_info, [&](storage::Page *page, RID &rid) {
            page->deleteRecord(rid);
            page->setDirty(true);
        });
    }
    return QueryResult();
}

QueryResult ExecutionEngine::executeUpdate(const nlohmann::json &plan) {
    std::string tableName = plan["tableName"];
    TableInfo *table_info = catalog_->get_table(tableName);
    if (!table_info) handleError("Table does not exist: " + tableName);

    auto updates = plan["updates"];
    scanTablePages(table_info, [&](storage::Page *page, RID &rid) {
        char buffer[PAGE_SIZE];
        uint16_t size;
        if (page->getRecord(rid, buffer, &size)) {
            for (const auto &upd : updates) {
                uint32_t colIndex = table_info->get_schema().get_column_index(upd["column"]);
                const MyColumn &col = table_info->get_schema().get_column(colIndex);
                if (col.type == TypeId::INTEGER) {
                    int32_t v = std::stoi(upd["value"].get<std::string>());
                    std::memcpy(buffer + col.offset, &v, sizeof(v));
                } else if (col.type == TypeId::BOOLEAN) {
                    bool v = (upd["value"] == "true" || upd["value"] == "1");
                    std::memcpy(buffer + col.offset, &v, sizeof(v));
                } else if (col.type == TypeId::VARCHAR) {
                    std::string v = upd["value"];
                    size_t copy_size = std::min(v.size(), (size_t)col.length);
                    std::memcpy(buffer + col.offset, v.c_str(), copy_size);
                }
            }
            page->updateRecord(rid, buffer, table_info->get_schema().get_length());
            page->setDirty(true);
        }
    });
    return QueryResult();
}

// 扫描单个 Page 所有有效记录
//     void ExecutionEngine::scanSinglePage(PageID pid, const Schema &schema, QueryResult &result) {
//         storage::Page* page = bufferManager_->fetchPage(pid);
//     uint16_t slot_count = page->getSlotCount();
//
//     std::cout << "[DEBUG scan] page_id=" << pid
//               << " slot_count=" << slot_count << std::endl;
//
//     char buf[PAGE_SIZE];
//     uint16_t size;
//
//     for (uint16_t slot = 0; slot < slot_count; slot++) {
//         if (page->getSlotSize(slot) == 0) continue;
//
//         RID rid;
//         rid.page_id = pid;
//         rid.slot_num = slot;
//
//         if (!page->getRecord(rid, buf, &size)) {
//             std::cout << "[DEBUG] getRecord failed for slot " << slot << std::endl;
//             continue;
//         }
//
//         std::cout << "[DEBUG] Read slot=" << slot
//                   << " size=" << size << std::endl;
//
//         QueryResult::Row row;
//         size_t offset = 0;
//
//         for (size_t col_idx = 0; col_idx < schema.get_column_count(); col_idx++) {
//             const MyColumn &col = schema.get_column(col_idx);
//             if (col.get_type() == TypeId::INTEGER) {
//                 int val;
//                 std::memcpy(&val, buf + offset, sizeof(int));
//                 offset += sizeof(int);
//                 row.push_back(std::to_string(val));
//             }
//             else if (col.get_type() == TypeId::VARCHAR) {
//                 uint32_t str_len;
//                 std::memcpy(&str_len, buf + offset, sizeof(uint32_t));
//                 offset += sizeof(uint32_t);
//                 std::string str(buf + offset, str_len);
//                 offset += str_len;
//                 row.push_back(str);
//             }
//         }
//
//         result.addRow(row);
//     }
// }
    void ExecutionEngine::scanSinglePage(PageID pid, const Schema &schema, QueryResult &result) {
    storage::Page* page = bufferManager_->fetchPage(pid);
    uint16_t slot_count = page->getSlotCount();

    std::cout << "[DEBUG scan] page_id=" << pid
              << " slot_count=" << slot_count << std::endl;

    char buf[PAGE_SIZE];
    uint16_t size;

    for (uint16_t slot = 0; slot < slot_count; slot++) {
        if (page->getSlotSize(slot) == 0) continue;

        RID rid{pid, slot};

        if (!page->getRecord(rid, buf, &size)) {
            std::cout << "[DEBUG] getRecord failed for slot " << slot << std::endl;
            continue;
        }

        std::cout << "[DEBUG] Read slot=" << slot << " size=" << size << std::endl;

        QueryResult::Row row;
        size_t offset = 0;

        for (uint32_t col_idx = 0; col_idx < schema.get_column_count(); col_idx++) {
            const MyColumn &col = schema.get_column(static_cast<uint32_t>(col_idx));

            if (col.type == TypeId::INTEGER) {
                int32_t val;
                std::memcpy(&val, buf + offset, sizeof(int32_t));
                offset += sizeof(int32_t);
                row.push_back(std::to_string(val));
            }
            else if (col.type == TypeId::BOOLEAN) {
                bool val;
                std::memcpy(&val, buf + offset, sizeof(bool));
                offset += sizeof(bool);
                row.push_back(val ? "true" : "false");
            }
            else if (col.type == TypeId::VARCHAR) {
                // 直接定长读取
                std::string str(buf + offset, col.length);
                // 去掉填充的 '\0'
                str.erase(str.find_first_of('\0'));
                offset += col.length;
                row.push_back(str);
            }
        }

        result.addRow(row);
    }
}



QueryResult ExecutionEngine::executePlan(const nlohmann::json &plan) {
    std::string type = plan["type"];
    if (type == "CreateTable") return executeCreateTable(plan);
    else if (type == "Insert") return executeInsert(plan);
    else if (type == "Select") return executeSelect(plan);
    else if (type == "Delete") return executeDelete(plan);
    else if (type == "Update") return executeUpdate(plan);
    else throw std::runtime_error("Unsupported execution plan type: " + type);
}

} // namespace minidb