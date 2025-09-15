#ifndef MINIDB_BPLUS_TREE_H
#define MINIDB_BPLUS_TREE_H

#include "storage/Pager.h"
#include "common/Types.h"
#include "common/Value.h"
#include "engine/bPlusTree/bplus_tree_page.h"
#include <memory>
#include <vector>

namespace minidb {
    namespace engine {

        class BPlusTree {
        public:
            BPlusTree(std::shared_ptr<storage::Pager> pager,
                      PageID root_page_id,
                      TypeId key_type);

            // 核心操作
            bool insert(const Value& key, const RID& rid);
            RID search(const Value& key);
            std::vector<RID> range_search(const Value& begin, const Value& end);
            bool remove(const Value& key);

            // 树信息
            PageID get_root_page_id() const { return root_page_id_; }
            void set_root_page_id(PageID page_id) { root_page_id_ = page_id; }

            TypeId get_key_type() const { return key_type_; }

            // 统计信息
            uint32_t get_height() const;
            uint32_t get_node_count() const;

        private:
            std::shared_ptr<storage::Pager> pager_;
            PageID root_page_id_;
            TypeId key_type_;

            // 辅助方法
            BPlusTreePage get_node(PageID page_id);
            void release_node(PageID page_id, bool is_dirty);

            // 查找辅助
            PageID find_leaf_page(const Value& key);

            // 插入辅助 - 修改后的接口
            bool insert_into_leaf(PageID leaf_page_id, const Value& key, const RID& rid);
            bool split_leaf_node(PageID leaf_page_id, Value* promoted_key, PageID* new_page_id);
            bool split_internal_node(PageID internal_page_id, Value* promoted_key, PageID* new_page_id);

            // 递归操作 - 修改后的接口
            bool insert_recursive(PageID current_page_id, const Value& key,
                                 const RID& rid, Value* promoted_key,
                                 PageID* new_child_page_id, bool* need_split);

            // 新节点创建
            PageID create_new_leaf_node();
            PageID create_new_internal_node();

            // 根节点处理
            bool create_new_root(PageID left_child_id, const Value& key, PageID right_child_id);
        };

    } // namespace engine
} // namespace minidb

#endif // MINIDB_BPLUS_TREE_H