// �������б��Ԫ��Ϣ

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "table_info.h"
#include "compiler/AST.h"

namespace minidb {

    /**
     * @brief Ŀ¼��������
     *
     * ���ݿ�ϵͳ��Ԫ���ݹ������ģ�����������б�Ĵ�����ɾ���Ͳ�ѯ��
     * ����ģ��ͨ�������ȡ���Ԫ��Ϣ����ȷ�������ݡ�
     * ���õ���ģʽ��ƣ�ȷ��Ԫ���ݵ�һ���ԡ�
     */
    class CatalogManager {
    public:
        CatalogManager() = default;  ///< Ĭ�Ϲ��캯��
        ~CatalogManager() = default; ///< Ĭ����������

        // �����ӿ�
        /**
         * @brief �����±�
         * @param table_name ����������Ψһ
         * @param schema ���ģʽ���壨����Ϣ��Լ���ȣ�
         * @return true-�����ɹ�, false-���Ѵ��ڻ򴴽�ʧ��
         */
        bool create_table(const std::string& table_name, const Schema& schema);

        /**
         * @brief ɾ����
         * @param table_name Ҫɾ���ı���
         * @return true-ɾ���ɹ�, false-������
         */
        bool drop_table(const std::string& table_name);

        /**
         * @brief �����Ƿ����
         * @param table_name Ҫ���ı���
         * @return true-�����, false-������
         */
        bool table_exists(const std::string& table_name) const;

        // ���ѯ�ӿڣ����Ľӿڣ�
        /**
         * @brief ��ȡ����Ϣ���ǳ����汾��
         * @param table_name ����
         * @return TableInfoָ�룬����������򷵻�nullptr
         * @note ����ͨ�����ص�ָ���޸ı���Ϣ��ʹ��ʱ�����
         */
        TableInfo* get_table(const std::string& table_name);

        /**
         * @brief ��ȡ����Ϣ�������汾��
         * @param table_name ����
         * @return TableInfo����ָ�룬����������򷵻�nullptr
         * @note ��֤���صı���Ϣ���ᱻ�޸ģ�����ֻ������
         */
        const TableInfo* get_table(const std::string& table_name) const;

        // �����ӿ�
        /**
         * @brief ��ȡ���б������б�
         * @return ����ĸ˳������ı�������
         * @note ��Ҫ������ʾ���ݿ��е����б�
         */
        std::vector<std::string> get_table_names() const;

        /**
         * @brief ��ȡ������
         * @return ��ǰ����ı�����
         */
        uint32_t get_table_count() const { return tables_.size(); }

        // ==================== 新增接口：AST集成 ====================
        /**
         * @brief 从AST创建新表
         * @param create_ast CREATE TABLE语句的AST节点
         * @return true-创建成功, false-表已存在或创建失�?
         */
        bool create_table_from_ast(const CreateTableAST& create_ast);

        /**
         * @brief 验证INSERT语句的AST是否有效
         * @param insert_ast INSERT语句的AST节点
         * @return true-语句有效, false-表不存在或列不匹�?
         */
        bool validate_insert_ast(const InsertAST& insert_ast) const;

        /**
         * @brief 验证SELECT语句的AST是否有效
         * @param select_ast SELECT语句的AST节点
         * @return true-语句有效, false-表不存在或列不存�?
         */
        bool validate_select_ast(const SelectAST& select_ast) const;

        /**
         * @brief 获取表的Schema信息（用于AST执行�?
         * @param table_name 表名
         * @return Schema的共享指针，如果表不存在则返回nullptr
         */
        std::shared_ptr<Schema> get_table_schema(const std::string& table_name) const;

    private:
        /// ����Ϣ�洢������������TableInfo��ӳ��
        /// ʹ��unique_ptr����ָ���Զ������ڴ棬�����ڴ�й©
        /// unordered_map�ṩO(1)ʱ�临�ӶȵĲ��Ҳ���
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;

        // ==================== 新增私有方法 ====================
        /**
         * @brief 将AST中的字符串类型转换为TypeId枚举
         * @param type_str 类型字符串（�? "INT", "STRING", "BOOLEAN"�?
         * @return 对应的TypeId枚举�?
         * @throw std::invalid_argument 如果类型字符串无�?
         */
        TypeId convert_ast_type_to_typeid(const std::string& type_str) const;

        /**
         * @brief 计算VARCHAR类型的合适长�?
         * @param type_str 类型字符串（�? "STRING" �? "VARCHAR(255)"�?
         * @return 计算得到的长�?
         */
        uint32_t calculate_varchar_length(const std::string& type_str) const;
    };

} // namespace minidb