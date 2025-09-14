#ifndef MINIDB_BPLUS_TREE_PAGE_H
#define MINIDB_BPLUS_TREE_PAGE_H

#include "storage/Page.h"
#include "common/Types.h"
#include "common/Value.h"
#include <cstdint>

namespace minidb {
namespace engine {

#pragma pack(push, 1)
struct BPlusNodeHeader {
    uint16_t is_leaf;        // 是否是叶子节点 (1/0)
    uint16_t key_count;      // 当前键的数量
    PageID next_page_id;     // 下一个叶子节点的页ID（用于范围扫描）
    // 注意：内部节点不需要next_page_id，但为了统一头部而保留
};
#pragma pack(pop)

class BPlusTreePage {
public:
    explicit BPlusTreePage(storage::Page* page);

    // 类型检查
    bool is_leaf() const { return header_.is_leaf == 1; }
    void set_leaf(bool is_leaf) { header_.is_leaf = is_leaf ? 1 : 0; }

    // 容量管理
    uint16_t get_key_count() const { return header_.key_count; }
    void set_key_count(uint16_t count) { header_.key_count = count; }

    PageID get_next_page_id() const { return header_.next_page_id; }
    void set_next_page_id(PageID page_id) { header_.next_page_id = page_id; }

    // 键操作
    Value get_key_at(int index) const;
    void set_key_at(int index, const Value& key);

    // 值操作（叶子节点存储RID，内部节点存储PageID）
    RID get_rid_at(int index) const;
    void set_rid_at(int index, const RID& rid);

    PageID get_child_page_id_at(int index) const;
    void set_child_page_id_at(int index, PageID page_id);

    // 查找操作
    int find_key_index(const Value& key) const;

    // 插入操作（简易版，假设有足够空间）
    bool insert_leaf_pair(const Value& key, const RID& rid);
    bool insert_internal_pair(const Value& key, PageID child_page_id);

    // 容量检查
    bool is_full() const;
    uint16_t get_free_space() const;

    // 序列化标记
    void mark_dirty() { page_->setDirty(true); }

private:
    storage::Page* page_;        // 对应的物理页
    BPlusNodeHeader header_;     // 节点头信息

    // 计算数据区域的偏移量
    char* get_data_start() const;
    size_t get_pair_size() const;
    size_t get_key_offset(int index) const;
    size_t get_value_offset(int index) const;

    // 序列化辅助函数
    void serialize_value(const Value& value, char* buffer) const;
    Value deserialize_value(const char* buffer, TypeId type) const;
};

} // namespace engine
} // namespace minidb

#endif // MINIDB_BPLUS_TREE_PAGE_H