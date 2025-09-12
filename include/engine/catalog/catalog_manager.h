// 管理所有表的元信息

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "table_info.h"
#include "compiler/AST.h"

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

        // ==================== 新增接口：AST集成 ====================
        /**
         * @brief 从AST创建新表
         * @param create_ast CREATE TABLE语句的AST节点
         * @return true-创建成功, false-表已存在或创建失败
         */
        bool create_table_from_ast(const CreateTableAST& create_ast);

        /**
         * @brief 验证INSERT语句的AST是否有效
         * @param insert_ast INSERT语句的AST节点
         * @return true-语句有效, false-表不存在或列不匹配
         */
        bool validate_insert_ast(const InsertAST& insert_ast) const;

        /**
         * @brief 验证SELECT语句的AST是否有效
         * @param select_ast SELECT语句的AST节点
         * @return true-语句有效, false-表不存在或列不存在
         */
        bool validate_select_ast(const SelectAST& select_ast) const;

        /**
         * @brief 获取表的Schema信息（用于AST执行）
         * @param table_name 表名
         * @return Schema的共享指针，如果表不存在则返回nullptr
         */
        std::shared_ptr<Schema> get_table_schema(const std::string& table_name) const;

    private:
        /// 表信息存储容器：表名到TableInfo的映射
        /// 使用unique_ptr智能指针自动管理内存，避免内存泄漏
        /// unordered_map提供O(1)时间复杂度的查找操作
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;

        // ==================== 新增私有方法 ====================
        /**
         * @brief 将AST中的字符串类型转换为TypeId枚举
         * @param type_str 类型字符串（如 "INT", "STRING", "BOOLEAN"）
         * @return 对应的TypeId枚举值
         * @throw std::invalid_argument 如果类型字符串无效
         */
        TypeId convert_ast_type_to_typeid(const std::string& type_str) const;

        /**
         * @brief 计算VARCHAR类型的合适长度
         * @param type_str 类型字符串（如 "STRING" 或 "VARCHAR(255)"）
         * @return 计算得到的长度
         */
        uint32_t calculate_varchar_length(const std::string& type_str) const;
    };

} // namespace minidb