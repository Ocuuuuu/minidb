// 一张表的完整元数据
#pragma once

#include <string>
#include <cstdint>
#include "schema.h"

namespace minidb {

    /**
     * @brief 表信息类
     *
     * 封装了一张表的完整元信息，包括表名和表结构模式。
     * 这是目录管理中的基本存储单元。
     */
    class TableInfo {
    public:
        /**
         * @brief 构造函数
         * @param table_name 表名
         * @param schema 表的结构模式
         * @param table_id 表ID，如果为0则自动生成
         */
        TableInfo(const std::string& table_name, const Schema& schema, PageID first_page_id, uint32_t table_id = 0);

        // 查询接口
        const std::string& get_table_name() const { return table_name_; } ///< 获取表名
        const Schema& get_schema() const { return schema_; } ///< 获取表结构模式
        uint32_t get_table_id() const { return table_id_; } ///< 获取表ID

        PageID getFirstPageID() const { return first_page_id_; } ///< 获取第一页ID

        /**
         * @brief 设置表ID（谨慎使用，主要用于反序列化）
         */
        void set_table_id(uint32_t table_id) { table_id_ = table_id; }

        /**
         * @brief 重载相等运算符
         */
        bool operator==(const TableInfo& other) const {
            return table_id_ == other.table_id_ &&
                   table_name_ == other.table_name_;
        }

        /**
         * @brief 重载不相等运算符
         */
        bool operator!=(const TableInfo& other) const {
            return !(*this == other);
        }

    private:
        std::string table_name_; ///< 表的名称
        Schema schema_;         ///< 表的结构定义
        uint32_t table_id_;     ///< 表的唯一标识符
        PageID first_page_id_;  ///< 表的第一页ID
    };

} // namespace minidb