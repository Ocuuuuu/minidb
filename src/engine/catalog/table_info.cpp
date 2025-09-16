#include "engine/catalog/table_info.h"
#include <atomic>

namespace minidb {

    // 静态原子变量，用于新建表生成唯一ID
    static std::atomic<uint32_t> global_table_id_counter{1};

    TableInfo::TableInfo(const std::string& table_name,
                         const Schema& schema,
                         PageID first_page_id,
                         uint32_t table_id)
        : table_name_(table_name),
          schema_(schema),
          first_page_id_(first_page_id) {

        if (table_id == 0) {
            // 新建表：自动分配表ID
            table_id_ = global_table_id_counter.fetch_add(1, std::memory_order_relaxed);
        } else {
            // 恢复表：使用指定ID
            table_id_ = table_id;
        }
    }

} // namespace minidb