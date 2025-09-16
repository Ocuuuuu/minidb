// 管理所有表的元信息

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "table_info.h"
#include "compiler/AST.h"

namespace minidb {

    //负责数据库表元数据的增删查改，确保数据一致性。
    class CatalogManager {
    public:
        CatalogManager() = default;  // 默认构造函数
        ~CatalogManager() = default; // 默认析构函数

        //操作接口
        bool create_table(const std::string& table_name, const Schema& schema);
        bool drop_table(const std::string& table_name);
        bool table_exists(const std::string& table_name) const;

        // 表查询接口（核心接口）
        TableInfo* get_table(const std::string& table_name);
        const TableInfo* get_table(const std::string& table_name) const;

        // 辅助接口
        std::vector<std::string> get_table_names() const;
        uint32_t get_table_count() const { return tables_.size(); }


        // AST集成接口
        bool create_table_from_ast(const CreateTableAST& create_ast);
        bool validate_insert_ast(const InsertAST& insert_ast) const;
        bool validate_select_ast(const SelectAST& select_ast) const;

        // 类型转换接口
        // 根据类型字符串返回对应的 TypeId
        TypeId getTypeIdFromString(const std::string& type_str) const;
        std::shared_ptr<Schema> get_table_schema(const std::string& table_name) const;

        TypeId getTypeIdFromAST(const std::string& type_str) const {
            return convert_ast_type_to_typeid(type_str);
        }

    private:
        // 表信息存储容器：表名到TableInfo的映射
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;

        TypeId convert_ast_type_to_typeid(const std::string& type_str) const;

        uint32_t calculate_varchar_length(const std::string& type_str) const;
    };

} // namespace minidb