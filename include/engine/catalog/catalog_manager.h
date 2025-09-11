//
// Created by 罗飞燕 on 2025/9/10.
//
//管理所有表的元信息

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
     */
    class CatalogManager {
    public:
        CatalogManager() = default;  ///< 默认构造函数
        ~CatalogManager() = default; ///< 默认析构函数

        // 表管理接口
        bool create_table(const std::string& table_name, const Schema& schema); ///< 创建新表
        bool drop_table(const std::string& table_name); ///< 删除表
        bool table_exists(const std::string& table_name) const; ///< 检查表是否存在

        // 表查询接口（核心接口）
        TableInfo* get_table(const std::string& table_name); ///< 获取表信息（非常量版本）
        const TableInfo* get_table(const std::string& table_name) const; ///< 获取表信息（常量版本）

        // 辅助接口
        std::vector<std::string> get_table_names() const; ///< 获取所有表名列表
        uint32_t get_table_count() const { return tables_.size(); } ///< 获取表数量

    private:
        /// 表信息存储：表名到TableInfo的映射，使用智能指针自动管理内存
        std::unordered_map<std::string, std::unique_ptr<TableInfo>> tables_;
    };

} // namespace minidb