#pragma once

#include "../include/common/Types.h"
#include "../include/common/Constants.h"
#include <string>
#include <cstring> // for std::memcpy

namespace minidb {
namespace catalog {

/**
 * @brief 索引元数据类
 * 
 * 用于存储和管理索引的元信息，包括索引名称、所属表、索引列、
 * 键类型和根页面ID等信息。
 */
class IndexMeta {
public:
    IndexMeta() = default;
    
    /**
     * @brief 构造函数
     * @param index_name 索引名称
     * @param table_name 所属表名
     * @param column_name 索引列名
     * @param key_type 键数据类型
     * @param root_page_id 根页面ID，默认为无效页面
     */
    IndexMeta(std::string index_name, 
              std::string table_name,
              std::string column_name, 
              TypeId key_type,
              PageID root_page_id = INVALID_PAGE_ID);

    // Getters
    const std::string& get_index_name() const { return index_name_; }
    const std::string& get_table_name() const { return table_name_; }
    const std::string& get_column_name() const { return column_name_; }
    TypeId get_key_type() const { return key_type_; }
    PageID get_root_page_id() const { return root_page_id_; }
    
    // Setters
    void set_root_page_id(PageID page_id) { root_page_id_ = page_id; }

    /**
     * @brief 序列化到缓冲区
     * @param buffer 目标缓冲区，必须至少有get_serialized_size()大小
     */
    void serialize(char* buffer) const;

    /**
     * @brief 从缓冲区反序列化
     * @param buffer 源缓冲区
     */
    void deserialize(const char* buffer);

    /**
     * @brief 获取序列化后的大小
     * @return 序列化所需的字节数
     */
    static size_t get_serialized_size();

    /**
     * @brief 转换为字符串表示
     * @return 索引信息的字符串描述
     */
    std::string to_string() const;

    /**
     * @brief 检查索引元数据是否有效
     * @return true-有效, false-无效
     */
    bool is_valid() const;

    // 运行时可用性验证（新增）
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