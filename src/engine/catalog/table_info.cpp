#include "../include/engine/catalog/table_info.h"
#include <atomic>

namespace minidb {

    // 静态原子计数器，用于生成唯一的表ID
    static std::atomic<uint32_t> global_table_id_counter{1};

    /**
     * @brief TableInfo构造函数实现
     *
     * 使用表名和模式初始化表信息，并生成唯一的表ID。
     * 避免使用哈希值，使用原子计数器确保ID唯一性。
     */
    TableInfo::TableInfo(const std::string& table_name, const Schema& schema, PageID first_page_id, uint32_t table_id)
        : table_name_(table_name), schema_(schema), first_page_id_(first_page_id) {  // 关键更改

        if (table_id == 0) {
            // 自动生成唯一表ID
            table_id_ = global_table_id_counter.fetch_add(1, std::memory_order_relaxed);
        } else {
            // 使用指定的表ID（主要用于从存储中恢复）
            table_id_ = table_id;
        }
    }

} // namespace minidbpace minidb