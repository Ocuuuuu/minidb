#include "engine/catalog/index_meta.h"
#include "common/Exception.h" // 确保这个头文件包含了DatabaseException的定义
#include <sstream>
#include <stdexcept>
#include <cstring> // 需要添加这个头文件用于std::strncpy

namespace minidb {
namespace catalog {

IndexMeta::IndexMeta(std::string index_name,
                     std::string table_name,
                     std::string column_name,
                     TypeId key_type,
                     PageID root_page_id)
    : index_name_(std::move(index_name)),
      table_name_(std::move(table_name)),
      column_name_(std::move(column_name)),
      key_type_(key_type),
      root_page_id_(root_page_id) {

    // 简单的有效性检查
    if (index_name_.empty()) {
        throw DatabaseException("Index name cannot be empty"); // 改为DatabaseException
    }
    if (table_name_.empty()) {
        throw DatabaseException("Table name cannot be empty"); // 改为DatabaseException
    }
    if (column_name_.empty()) {
        throw DatabaseException("Column name cannot be empty"); // 改为DatabaseException
    }
    if (key_type == TypeId::INVALID) {
        throw DatabaseException("Key type cannot be invalid"); // 改为DatabaseException
    }
}

void IndexMeta::serialize(char* buffer) const {
    if (buffer == nullptr) {
        throw DatabaseException("Buffer cannot be null for serialization"); // 改为DatabaseException
    }

    char* ptr = buffer;

    // 序列化 index_name_ (固定长度)
    std::strncpy(ptr, index_name_.c_str(), NAME_LENGTH);
    ptr[NAME_LENGTH - 1] = '\0'; // 确保以null结尾
    ptr += NAME_LENGTH;

    // 序列化 table_name_ (固定长度)
    std::strncpy(ptr, table_name_.c_str(), NAME_LENGTH);
    ptr[NAME_LENGTH - 1] = '\0';
    ptr += NAME_LENGTH;

    // 序列化 column_name_ (固定长度)
    std::strncpy(ptr, column_name_.c_str(), NAME_LENGTH);
    ptr[NAME_LENGTH - 1] = '\0';
    ptr += NAME_LENGTH;

    // 序列化 key_type_ (作为底层整型)
    *reinterpret_cast<std::underlying_type_t<TypeId>*>(ptr) =
        static_cast<std::underlying_type_t<TypeId>>(key_type_);
    ptr += sizeof(std::underlying_type_t<TypeId>);

    // 序列化 root_page_id_
    *reinterpret_cast<PageID*>(ptr) = root_page_id_;
}

void IndexMeta::deserialize(const char* buffer) {
    if (buffer == nullptr) {
        throw DatabaseException("Buffer cannot be null for deserialization"); // 改为DatabaseException
    }

    const char* ptr = buffer;

    // 反序列化 index_name_
    index_name_ = std::string(ptr, NAME_LENGTH);
    // 去除可能的多余null字符
    size_t null_pos = index_name_.find('\0');
    if (null_pos != std::string::npos) {
        index_name_.resize(null_pos);
    }
    ptr += NAME_LENGTH;

    // 反序列化 table_name_
    table_name_ = std::string(ptr, NAME_LENGTH);
    null_pos = table_name_.find('\0');
    if (null_pos != std::string::npos) {
        table_name_.resize(null_pos);
    }
    ptr += NAME_LENGTH;

    // 反序列化 column_name_
    column_name_ = std::string(ptr, NAME_LENGTH);
    null_pos = column_name_.find('\0');
    if (null_pos != std::string::npos) {
        column_name_.resize(null_pos);
    }
    ptr += NAME_LENGTH;

    // 反序列化 key_type_
    key_type_ = static_cast<TypeId>(
        *reinterpret_cast<const std::underlying_type_t<TypeId>*>(ptr));
    ptr += sizeof(std::underlying_type_t<TypeId>);

    // 反序列化 root_page_id_
    root_page_id_ = *reinterpret_cast<const PageID*>(ptr);
}

size_t IndexMeta::get_serialized_size() {
    return NAME_LENGTH * 3 + // 三个名称字段
           sizeof(std::underlying_type_t<TypeId>) + // key_type
           sizeof(PageID); // root_page_id
}

std::string IndexMeta::to_string() const {
    std::ostringstream oss;
    oss << "IndexMeta{"
        << "name='" << index_name_ << "', "
        << "table='" << table_name_ << "', "
        << "column='" << column_name_ << "', "
        << "key_type=" << static_cast<int>(key_type_) << ", "
        << "root_page_id=" << root_page_id_
        << "}";
    return oss.str();
}

    // 元数据完整性验证（永远不需要修改）
    bool IndexMeta::is_valid() const {
    return !index_name_.empty() &&
           !table_name_.empty() &&
           !column_name_.empty() &&
           key_type_ != TypeId::INVALID;
}

    // 运行时可用性验证（可能随存储系统变化）
    bool IndexMeta::is_ready_for_use() const {
    return is_valid() && root_page_id_ != INVALID_PAGE_ID;
}

} // namespace catalog
} // namespace minidb