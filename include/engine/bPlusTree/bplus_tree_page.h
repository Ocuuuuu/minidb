#ifndef MINIDB_BPLUS_TREE_PAGE_H
#define MINIDB_BPLUS_TREE_PAGE_H

#include "storage/Page.h"
#include "common/Types.h"
#include "common/Value.h"
#include <cstdint>
#include <vector>

namespace minidb {
namespace engine {

#pragma pack(push, 1)
struct BPlusNodeHeader {
    uint16_t is_leaf;        // 是否是叶子节点 (1/0)
    uint16_t key_count;      // 当前键的数量
    PageID next_page_id;     // 下一个叶子节点的页ID
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
    void set_key_count(uint16_t count) {
        header_.key_count = count;
        mark_dirty();
    }

    PageID get_next_page_id() const { return header_.next_page_id; }
    void set_next_page_id(PageID page_id) {
        header_.next_page_id = page_id;
        mark_dirty();
    }

    // 键操作
    Value get_key_at(int index) const;
    void set_key_at(int index, const Value& key);
    void remove_key_at(int index);

    // 值操作
    RID get_rid_at(int index) const;
    void set_rid_at(int index, const RID& rid);
    void remove_rid_at(int index);

    PageID get_child_page_id_at(int index) const;
    void set_child_page_id_at(int index, PageID page_id);
    void remove_child_page_id_at(int index);

    // 查找操作
    int find_key_index(const Value& key) const;

    // 插入操作
    bool insert_leaf_pair(const Value& key, const RID& rid);
    bool insert_internal_pair(const Value& key, PageID child_page_id);

    // 删除操作
    bool remove_leaf_pair(int index);
    bool remove_internal_pair(int index);

    // 容量检查
    bool is_full() const;
    bool is_underflow() const;
    uint16_t get_free_space() const;
    uint16_t get_max_capacity() const;

    // 序列化标记
    void mark_dirty() { page_->setDirty(true); }
    void save_header();

    // 调试信息
    void print_debug_info() const;

private:
    storage::Page* page_;        // 对应的物理页
    BPlusNodeHeader header_;     // 节点头信息

    // 计算数据区域的偏移量
    char* get_data_start() const;
    size_t get_key_size() const;
    size_t get_value_size() const;
    size_t get_pair_size() const;
    size_t get_key_offset(int index) const;
    size_t get_value_offset(int index) const;

    // 序列化辅助函数
    void serialize_key(const Value& key, char* buffer) const;
    Value deserialize_key(const char* buffer) const;
    void serialize_rid(const RID& rid, char* buffer) const;
    RID deserialize_rid(const char* buffer) const;
    void serialize_page_id(PageID page_id, char* buffer) const;
    PageID deserialize_page_id(const char* buffer) const;

    // 内部辅助方法
    void shift_keys_right(int start_index, int count = 1);
    void shift_keys_left(int start_index, int count = 1);
    void shift_values_right(int start_index, int count = 1);
    void shift_values_left(int start_index, int count = 1);
};

} // namespace engine
} // namespace minidb

#endif // MINIDB_BPLUS_TREE_PAGE_H