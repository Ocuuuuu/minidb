// 管理所有表的元信息

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "table_info.h"

namespace minidb {

    /**
     * @brief 目录管理器类
     *
     * 数据库系统的元数据管理中心，负责管理所有表的创建、删除和查询。
     * 其他模块通过此类获取表的元信息来正确操作数据。
     * 采用单例模式设计，确保元数据的一致性。
     */
    class CatalogManager {
    public:
        CatalogManager() = default;  ///< 默认构造函数
        ~CatalogManager() = default; ///< 默认析构函数

        // 表管理接口
        /**
         * @brief 创建新表
         * @param table_name 表名，必须唯一
         * @param schema 表的模式定义（列信息、约束等）
         * @return true-创建成功, false-表已存在或创建失败
         */
        bool create_table(const std::string& table_name, const Schema& schema);

        /**
         * @brief 删除表
         * @param table_name 要删除的表名
         * @return true-删除成功, false-表不存在
         */
        bool drop_table(const std::string& table_name);

        /**
         * @brief 检查表是否存在
         * @param table_name 要检查的表名
         * @return true-表存在, false-表不存在
         */
        bool table_exists(const std::string& table_name) const;

        // 表查询接口（核心接口）
        /**
         * @brief 获取表信息（非常量版本）
         * @param table_name 表名
         * @return TableInfo指针，如果表不存在则返回nullptr
         * @note 允许通过返回的指针修改表信息，使用时需谨慎
         */
        TableInfo* get_table(const std::string& table_name);

        /**
         * @brief 获取表信息（常量版本）
         * @param table_name 表名
         * @return TableInfo常量指针，如果表不存在则返回nullptr
         * @note 保证返回的表信息不会被修改，用于只读操作
         */
        const TableInfo* get_table(const std::string& table_name) const;

        // 辅助接口
        /**
         * @brief 获取所有表名的列表
         * @return 按字母顺序排序的表名向量
         * @note 主要用于显示数据库中的所有表
         */
        std::vector<std::string> get_table_names() const;

        /**
         * @brief 获取表数量
         * @return 当前管理的表总数
         */
        uint32_t get_table_count() const { return tables_.size(); }

    private:
        /// 表信息存储容器：表名到TableInfo的映射
        /// 使用unique_ptr智能指针自动管理内存，避免内存泄漏
        /// unordered_map提供O(1)时间复杂度的查找操作
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;
    };

} // namespace minidb