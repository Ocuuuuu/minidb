#pragma once

#include "../include/common/Types.h"
#include "../include/common/Constants.h"
#include <string>
#include <cstring> // for std::memcpy

namespace minidb {
namespace catalog {

// 索引元数据类：管理索引名称、表名、列名、键类型和根页面ID
class IndexMeta {
public:
    IndexMeta() = default;

    // 构造函数
    IndexMeta(std::string index_name, 
              std::string table_name,
              std::string column_name, 
              TypeId key_type,
              PageID root_page_id = INVALID_PAGE_ID);

    // Getter接口
    const std::string& get_index_name() const { return index_name_; }
    const std::string& get_table_name() const { return table_name_; }
    const std::string& get_column_name() const { return column_name_; }
    TypeId get_key_type() const { return key_type_; }
    PageID get_root_page_id() const { return root_page_id_; }
    
    // Setter接口
    void set_root_page_id(PageID page_id) { root_page_id_ = page_id; }

    // 序列化/反序列化接口
    void serialize(char* buffer) const;
    void deserialize(const char* buffer);
    static size_t get_serialized_size();

    // 辅助功能
    std::string to_string() const;
    bool is_valid() const;
    // 运行时可用性验证
    bool is_ready_for_use() const;

private:
    std::string index_name_;
    std::string table_name_; 
    std::string column_name_;
    TypeId key_type_{TypeId::INVALID};
    PageID root_page_id_{INVALID_PAGE_ID};

    // 序列化字段的固定长度（为了对齐和固定大小）
    static constexpr size_t NAME_LENGTH = 64; // 名称字段的最大长度
};

} // namespace catalog
} // namespace minidb