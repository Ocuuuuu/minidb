// �������б��Ԫ��Ϣ

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "table_info.h"

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

    private:
        /// ����Ϣ�洢������������TableInfo��ӳ��
        /// ʹ��unique_ptr����ָ���Զ������ڴ棬�����ڴ�й©
        /// unordered_map�ṩO(1)ʱ�临�ӶȵĲ��Ҳ���
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;
    };

} // namespace minidb