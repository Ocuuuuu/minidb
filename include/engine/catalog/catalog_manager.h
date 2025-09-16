// �������б��Ԫ��Ϣ

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "table_info.h"
#include "compiler/AST.h"

namespace minidb {

    //�������ݿ��Ԫ���ݵ���ɾ��ģ�ȷ������һ���ԡ�
    class CatalogManager {
    public:
        CatalogManager() = default;  // Ĭ�Ϲ��캯��
        ~CatalogManager() = default; // Ĭ����������

        //�����ӿ�
        bool create_table(const std::string& table_name, const Schema& schema);
        bool drop_table(const std::string& table_name);
        bool table_exists(const std::string& table_name) const;

        // ���ѯ�ӿڣ����Ľӿڣ�
        TableInfo* get_table(const std::string& table_name);
        const TableInfo* get_table(const std::string& table_name) const;

        // �����ӿ�
        std::vector<std::string> get_table_names() const;
        uint32_t get_table_count() const { return tables_.size(); }


        // AST���ɽӿ�
        bool create_table_from_ast(const CreateTableAST& create_ast);
        bool validate_insert_ast(const InsertAST& insert_ast) const;
        bool validate_select_ast(const SelectAST& select_ast) const;

        // ����ת���ӿ�
        // ���������ַ������ض�Ӧ�� TypeId
        TypeId getTypeIdFromString(const std::string& type_str) const;
        std::shared_ptr<Schema> get_table_schema(const std::string& table_name) const;

        TypeId getTypeIdFromAST(const std::string& type_str) const {
            return convert_ast_type_to_typeid(type_str);
        }

    private:
        // ����Ϣ�洢������������TableInfo��ӳ��
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;

        TypeId convert_ast_type_to_typeid(const std::string& type_str) const;

        uint32_t calculate_varchar_length(const std::string& type_str) const;
    };

} // namespace minidb