#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include "../compiler/AST.h"
#include "../engine/catalog/catalog_manager.h"
#include "storage/BufferManager.h"
#include <memory>
#include <string>
#include <vector>
#include "common/QueryResult.h"

namespace minidb {

class ExecutionEngine {
public:

    // SeqScanAST 用于表示顺序扫描
    struct SeqScanAST {
        std::string tableName; // 要扫描的表名
    };

    // FilterAST 用于表示过滤条件
    struct FilterAST {
        SeqScanAST seqScan; // 对应的顺序扫描
        Condition condition; // 用于过滤的条件
    };

    // ProjectAST 用于表示投影
    struct ProjectAST {
        SeqScanAST seqScan;           // 对应的顺序扫描
        std::vector<std::string> columns; // 需要返回的列名
    };




    ExecutionEngine(std::shared_ptr<minidb::CatalogManager> catalog,
                    std::shared_ptr<minidb::storage::BufferManager> bufferManager)
        : catalog_(catalog), bufferManager_(bufferManager) {}


    bool executeCreateTable(const CreateTableAST& create_ast);

    bool executeInsert(const InsertAST& insert_ast);

    QueryResult executeSelect(const SelectAST& select_ast);

    bool executeDelete(const DeleteAST& delete_ast);

    QueryResult executeSeqScan(const SeqScanAST& seqScanAST);

    QueryResult executeFilter(const FilterAST& filterAST) ;

    QueryResult executeProject(const ProjectAST& projectAST);
private:
    std::shared_ptr<minidb::CatalogManager> catalog_;
    std::shared_ptr<minidb::storage::BufferManager> bufferManager_;

    bool createTable(const std::string& tableName, const Schema& schema);

    bool insertIntoTable(const std::string& tableName, const std::vector<std::string>& values);

    QueryResult selectFromTable(const SelectAST& select_ast);

    bool deleteFromTable(const DeleteAST& delete_ast);
};
}

#endif // EXECUTION_ENGINE_H