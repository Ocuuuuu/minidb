#ifndef MINIDB_BPLUS_TREE_H
#define MINIDB_BPLUS_TREE_H

#include "common/Value.h"
#include "common/Types.h"
#include "storage/Pager.h"
#include <memory>
#include <vector>

namespace minidb {
namespace engine {

class BPlusTreePage; // 前向声明

// B+树节点常量定义
// constexpr int LEAF_NODE_MAX_KEYS = 32;      // 叶子节点最大键数
// constexpr int INTERNAL_NODE_MAX_KEYS = 32;  // 内部节点最大键数


    // B+树节点常量定义
    constexpr int LEAF_NODE_MAX_KEYS = 4;      // 叶子节点最大键数（调小触发分裂）
    constexpr int INTERNAL_NODE_MAX_KEYS = 4;  // 内部节点最大键数

class BPlusTree {
public:
    BPlusTree(std::shared_ptr<storage::Pager> pager,
              PageID root_page_id,
              TypeId key_type);

    // 基本操作
    bool insert(const Value& key, const RID& rid);
    RID search(const Value& key) const;
    std::vector<RID> range_search(const Value& begin, const Value& end) const;
    bool remove(const Value& key);

    // 树信息
    uint32_t get_height() const;
    uint32_t get_node_count() const;
    PageID get_root_page_id() const { return root_page_id_; }

    // 调试工具
    void print_tree() const;

private:
    std::shared_ptr<storage::Pager> pager_;
    PageID root_page_id_;
    TypeId key_type_;
    mutable uint32_t node_count_ = 0;

    /**
     * 根据键类型计算键的大小（字节）
     * @param type 键的数据类型
     * @return 键的大小（字节数），VARCHAR 返回 0 表示变长
     */
    uint16_t calculate_key_size(TypeId type) const;

    // 辅助方法
    Value get_min_value_for_type(TypeId type) const;
    void validate_key_type(const Value& key) const;

    // 页面管理
    std::unique_ptr<BPlusTreePage> get_node(PageID page_id) const;
    void release_node(PageID page_id, bool is_dirty) const;

    // 搜索方法
    PageID find_leaf_page(const Value& key) const;
    PageID find_first_leaf_page() const;

    // 插入相关
    bool insert_recursive(PageID current_page_id, const Value& key,
                         const RID& rid, Value* promoted_key,
                         PageID* new_child_page_id, bool* need_split);
    bool split_leaf_node(PageID leaf_page_id, const Value& new_key, const RID& new_rid,
                        Value* promoted_key, PageID* new_page_id);
    bool split_internal_node(PageID internal_page_id, const Value& new_key, PageID new_child_id,
                           Value* promoted_key, PageID* new_page_id);
    bool create_new_root(PageID left_child_id, const Value& key, PageID right_child_id);

    // 删除相关
    bool remove_recursive(PageID current_page_id, const Value& key, bool* need_merge);
    bool remove_from_leaf(PageID leaf_page_id, const Value& key);
    bool handle_underflow(PageID parent_page_id, PageID page_id, int index);

    // 节点创建
    PageID create_new_leaf_node();
    PageID create_new_internal_node();
};

} // namespace engine
} // namespace minidb

#endif // MINIDB_BPLUS_TREE_H