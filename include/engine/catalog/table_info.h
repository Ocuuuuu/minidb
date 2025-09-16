#pragma once

#include <string>
#include <cstdint>
#include "schema.h"

namespace minidb {

    /**
     * @brief 数据库表完整元信息
     * @details 保存表名、Schema、表ID、首数据页ID
     */
    class TableInfo {
    public:
        /**
         * @brief 构造函数
         * @param table_name 表名
         * @param schema 表模式
         * @param first_page_id 首数据页ID
         * @param table_id 表ID（0表示新建时自动分配）
         */
        TableInfo(const std::string& table_name,
                  const Schema& schema,
                  PageID first_page_id,
                  uint32_t table_id = 0);

        // ======== 元数据访问接口 ========

        const std::string& get_table_name() const { return table_name_; }
        const Schema& get_schema() const { return schema_; }
        uint32_t get_table_id() const { return table_id_; }

        PageID getFirstPageID() const { return first_page_id_; }

        // 设置首数据页ID（用于建表时绑定物理页 或 恢复时还原）
        void setFirstPageID(PageID pid) { first_page_id_ = pid; }

        // 设置表ID（恢复时可用）
        void set_table_id(uint32_t table_id) { table_id_ = table_id; }

        // 比较运算符
        bool operator==(const TableInfo& other) const {
            return table_id_ == other.table_id_ &&
                   table_name_ == other.table_name_;
        }

        bool operator!=(const TableInfo& other) const { return !(*this == other); }

    private:
        std::string table_name_; // 表名
        Schema schema_;          // 表结构定义
        uint32_t table_id_;      // 表唯一标识
        PageID first_page_id_;   // 表首数据页ID
    };

} // namespace minidb