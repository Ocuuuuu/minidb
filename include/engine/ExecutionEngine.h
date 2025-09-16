#pragma once

#include "engine/catalog/catalog_manager.h"
#include "../include/storage/BufferManager.h"
#include "../include/json.hpp"
#include "common/QueryResult.h"
#include "common/Value.h"

#include <memory>
#include <stdexcept>
#include <functional>
#include <vector>
#include <string>

namespace minidb {

    class ExecutionEngine {
    public:
        ExecutionEngine(std::shared_ptr<CatalogManager> catalog,
                        std::shared_ptr<storage::BufferManager> bufferManager)
            : catalog_(std::move(catalog)), bufferManager_(std::move(bufferManager)) {}

        // SQL执行接口
        QueryResult executeCreateTable(const nlohmann::json &plan);
        QueryResult executeInsert(const nlohmann::json &plan);
        QueryResult executeSelect(const nlohmann::json &plan);

        QueryResult executeDelete(const nlohmann::json &plan);
        QueryResult executeUpdate(const nlohmann::json &plan);

        // 执行入口
        QueryResult executePlan(const nlohmann::json &plan);

    private:
        std::shared_ptr<CatalogManager> catalog_;
        std::shared_ptr<storage::BufferManager> bufferManager_;

        void handleError(const std::string &message) const {
            throw std::runtime_error(message);
        }

        // 内部工具函数：按页扫描表
        void scanTablePages(const TableInfo *table_info,
                            const std::function<void(storage::Page *, RID &)> &callback);

        // 内部工具函数：表空间扩展
        PageID appendNewPageToTable(TableInfo *table_info);


        void scanSinglePage(PageID pid, const Schema &schema, QueryResult &result);

    };

} // namespace minidb