#include "engine/bPlusTree/bplus_tree.h"

#include <functional>

#include "engine/bPlusTree/bplus_tree_page.h"
#include <stdexcept>
#include <iostream>

namespace minidb {
namespace engine {

BPlusTree::BPlusTree(std::shared_ptr<storage::Pager> pager,
                     PageID root_page_id,
                     TypeId key_type)
    : pager_(pager), root_page_id_(root_page_id), key_type_(key_type) {
    if (!pager_->isValidPage(root_page_id_)) {
        root_page_id_ = create_new_leaf_node();
    }
}

bool BPlusTree::insert(const Value& key, const RID& rid) {
    validate_key_type(key);
    Value promoted_key;
    PageID new_child_page_id;
    bool need_split = false;

    return insert_recursive(root_page_id_, key, rid, &promoted_key, &new_child_page_id, &need_split);
}

RID BPlusTree::search(const Value& key) const {
    PageID leaf_page_id = find_leaf_page(key);
    auto leaf_page = get_node(leaf_page_id);

    int index = leaf_page->find_key_index(key);
    if (index >= 0 && leaf_page->is_leaf()) {
        return leaf_page->get_rid_at(index);
    }

    return RID::invalid();
}

std::vector<RID> BPlusTree::range_search(const Value& begin, const Value& end) const {
    std::vector<RID> results;
    PageID leaf_page_id = find_leaf_page(begin);

    while (leaf_page_id != -1) {
        auto leaf_page = get_node(leaf_page_id);
        int start_index = std::max(0, leaf_page->find_key_index(begin));
        for (int i = start_index; i < leaf_page->get_key_count(); ++i) {
            Value key = leaf_page->get_key_at(i);
            if (key > end) return results;

            if (key >= begin) {
                results.push_back(leaf_page->get_rid_at(i));
            }
        }

        leaf_page_id = leaf_page->get_next_page_id();
    }
    return results;
}

bool BPlusTree::remove(const Value& key) {
    validate_key_type(key);
    bool need_merge = false;

    return remove_recursive(root_page_id_, key, &need_merge);
}

uint32_t BPlusTree::get_height() const {
    uint32_t height = 0;
    PageID page_id = root_page_id_;

    while (page_id != -1) {
        auto page = get_node(page_id);
        ++height;
        if (page->is_leaf()) break;
        page_id = page->get_child_page_id_at(0);
    }
    return height;
}

uint32_t BPlusTree::get_node_count() const {
    // 返回节点的计数器，不包括非分配的页
    return node_count_;
}

    void BPlusTree::print_tree() const {
    std::cout << "BPlusTree (Root Page ID: " << root_page_id_ << ")\n";

    std::function<void(PageID, int)> dfs = [&](PageID page_id, int depth) {
        if (page_id == INVALID_PAGE_ID) return;

        auto page = get_node(page_id);
        std::string indent(depth * 4, ' '); // 每层4个空格

        std::cout << indent
                  << "|-- Node(PageID=" << page_id << ", "
                  << (page->is_leaf() ? "Leaf" : "Internal")
                  << ", Keys=" << page->get_key_count()
                  << ")\n";

        // 打印 keys 和对应值
        for (int i = 0; i < page->get_key_count(); i++) {
            std::cout << indent << "    Key[" << i << "]: " << page->get_key_at(i).toString();
            if (page->is_leaf()) {
                std::cout << " -> RID: " << page->get_rid_at(i).toString();
            } else {
                std::cout << " -> ChildPage: " << page->get_child_page_id_at(i);
            }
            std::cout << "\n";
        }

        if (!page->is_leaf()) {
            // 递归打印最后一个子节点
            PageID last_child = page->get_child_page_id_at(page->get_key_count());
            for (int i = 0; i <= page->get_key_count(); i++) {
                PageID child_id = (i == page->get_key_count())
                                    ? last_child
                                    : page->get_child_page_id_at(i);
                dfs(child_id, depth + 1);
            }
        }
    };

    dfs(root_page_id_, 0);
}


uint16_t BPlusTree::calculate_key_size(TypeId type) const {
    switch (type) {
        case TypeId::BOOLEAN: return sizeof(bool);
        case TypeId::INTEGER: return sizeof(int32_t);
        case TypeId::VARCHAR: return 0; // Variable length
        default: throw std::runtime_error("Unsupported type for key size calculation");
    }
}

Value BPlusTree::get_min_value_for_type(TypeId type) const {
    switch (type) {
        case TypeId::BOOLEAN: return Value(false); // Minimum boolean
        case TypeId::INTEGER: return Value(std::numeric_limits<int32_t>::min());
        case TypeId::VARCHAR: return Value(""); // Minimum string
        default: throw std::runtime_error("Unsupported type for min value calculation");
    }
}

void BPlusTree::validate_key_type(const Value& key) const {
    if (key.getType() != key_type_) {
        throw std::invalid_argument("Key type mismatch");
    }
}



    std::unique_ptr<BPlusTreePage> BPlusTree::get_node(PageID page_id) const {
    if (page_id == INVALID_PAGE_ID) {
        throw std::runtime_error("B+Tree tried to access INVALID_PAGE_ID (-1)!");
    }
    auto page = pager_->getPage(page_id);
    return std::make_unique<BPlusTreePage>(page);
}


void BPlusTree::release_node(PageID page_id, bool is_dirty) const {
    pager_->releasePage(page_id, is_dirty);
}

PageID BPlusTree::find_leaf_page(const Value& key) const {
    PageID page_id = root_page_id_;
    while (true) {
        auto page = get_node(page_id);

        if (page->is_leaf()) return page_id;
        int index = page->find_key_index(key);

        if (index < 0) index = -index - 1; // If key not found, select first greater key
        page_id = page->get_child_page_id_at(index);
    }
}

PageID BPlusTree::find_first_leaf_page() const {
    PageID page_id = root_page_id_;
    while (true) {
        auto page = get_node(page_id);
        if (page->is_leaf()) return page_id;
        page_id = page->get_child_page_id_at(0);
    }
}

bool BPlusTree::insert_recursive(PageID current_page_id, const Value& key,
                                 const RID& rid, Value* promoted_key,
                                 PageID* new_child_page_id, bool* need_split) {
    auto current_page = get_node(current_page_id);
    if (current_page->is_leaf()) {
        if (current_page->insert_leaf_pair(key, rid)) {
            release_node(current_page_id, true);
            return true;
        }
        return split_leaf_node(current_page_id, key, rid, promoted_key, new_child_page_id);
    } else {
        int index = current_page->find_key_index(key);
        if (index < 0) index = -index - 1;

        PageID child_page_id = current_page->get_child_page_id_at(index);
        bool result = insert_recursive(child_page_id, key, rid, promoted_key, new_child_page_id, need_split);

        if (*need_split) {
            *need_split = false;
            if (current_page->insert_internal_pair(*promoted_key, *new_child_page_id)) {
                release_node(current_page_id, true);
            } else {
                return split_internal_node(current_page_id, *promoted_key, *new_child_page_id, promoted_key, new_child_page_id);
            }
        }
        return result;
    }
}

    bool BPlusTree::split_leaf_node(PageID leaf_page_id, const Value& new_key, const RID& new_rid,
                                    Value* promoted_key, PageID* new_page_id) {
    auto leaf_page = get_node(leaf_page_id);
    *new_page_id = create_new_leaf_node();
    auto new_leaf_page = get_node(*new_page_id);

    int total_keys = leaf_page->get_key_count();
    int split_index = total_keys / 2;

    // 复制到新节点
    for (int i = split_index; i < total_keys; ++i) {
        Value key = leaf_page->get_key_at(i);
        RID rid = leaf_page->get_rid_at(i);
        new_leaf_page->insert_leaf_pair(key, rid);
    }

    // 清理原节点多余部分
    while (leaf_page->get_key_count() > split_index) {
        leaf_page->remove_leaf_pair(split_index);
    }

    new_leaf_page->set_next_page_id(leaf_page->get_next_page_id());
    leaf_page->set_next_page_id(*new_page_id);

    *promoted_key = new_leaf_page->get_key_at(0);

    if (new_key < *promoted_key) {
        leaf_page->insert_leaf_pair(new_key, new_rid);
    } else {
        new_leaf_page->insert_leaf_pair(new_key, new_rid);
    }

    release_node(leaf_page_id, true);
    release_node(*new_page_id, true);
    return true;
}


bool BPlusTree::split_internal_node(PageID internal_page_id, const Value& new_key, PageID new_child_id,
                                    Value* promoted_key, PageID* new_page_id) {
    auto internal_page = get_node(internal_page_id);
    *new_page_id = create_new_internal_node();
    auto new_internal_page = get_node(*new_page_id);

    int total_keys = internal_page->get_key_count();
    int split_index = (total_keys / 2);

    // 移动一半的键到新页面
    for (int i = split_index + 1; i < total_keys; ++i) {
        Value key = internal_page->get_key_at(i);
        PageID child_id = internal_page->get_child_page_id_at(i);
        new_internal_page->insert_internal_pair(key, child_id);
    }

    *promoted_key = internal_page->get_key_at(split_index);
    new_internal_page->set_child_page_id_at(0, internal_page->get_child_page_id_at(split_index + 1));

    internal_page->set_key_count(split_index);
    if (new_key < *promoted_key) {
        internal_page->insert_internal_pair(new_key, new_child_id);
    } else {
        new_internal_page->insert_internal_pair(new_key, new_child_id);
    }

    release_node(internal_page_id, true);
    release_node(*new_page_id, true);
    return true;
}

bool BPlusTree::create_new_root(PageID left_child_id, const Value& key, PageID right_child_id) {
    root_page_id_ = create_new_internal_node();
    auto root_page = get_node(root_page_id_);

    root_page->insert_internal_pair(key, right_child_id);
    root_page->set_child_page_id_at(0, left_child_id);

    release_node(root_page_id_, true);
    return true;
}

bool BPlusTree::remove_recursive(PageID current_page_id, const Value& key, bool* need_merge) {
    auto current_page = get_node(current_page_id);
    if (current_page->is_leaf()) {
        if (current_page->remove_leaf_pair(current_page->find_key_index(key))) {
            release_node(current_page_id, true);
            *need_merge = current_page->is_underflow();
            return true;
        }
        return false;
    } else {
        int index = current_page->find_key_index(key);
        if (index < 0) index = -index - 1;

        PageID child_page_id = current_page->get_child_page_id_at(index);
        bool result = remove_recursive(child_page_id, key, need_merge);

        if (*need_merge) {
            *need_merge = false;
            handle_underflow(current_page_id, child_page_id, index);
        }
        return result;
    }
}

bool BPlusTree::remove_from_leaf(PageID leaf_page_id, const Value& key) {
    auto leaf_page = get_node(leaf_page_id);
    if (leaf_page->remove_leaf_pair(leaf_page->find_key_index(key))) {
        release_node(leaf_page_id, true);
        return leaf_page->is_underflow();
    }
    return false;
}

bool BPlusTree::handle_underflow(PageID parent_page_id, PageID page_id, int index) {
    // Simple implementation to handle underflow
    // Note: Incomplete, as this often requires borrowing or merging of nodes

    auto parent_page = get_node(parent_page_id);
    auto current_page = get_node(page_id);

    if (current_page->is_leaf()) {
        // Handle leaf underflow (simplified)
        // Just delete the leaf page if possible (unsafe in real implementation)
        return pager_->isValidPage(page_id);
    } else {
        // Handle internal node underflow (simplified)
        return false; // Returning false for now
    }
}

PageID BPlusTree::create_new_leaf_node() {
    PageID page_id = pager_->allocatePage();
    auto page = pager_->getPage(page_id);

    BPlusTreePage bpt_page(page);
    bpt_page.initialize_page();
    bpt_page.set_leaf(true);
    bpt_page.set_key_type(key_type_);
    release_node(page_id, true);
    node_count_++;
    return page_id;
}

PageID BPlusTree::create_new_internal_node() {
    PageID page_id = pager_->allocatePage();
    auto page = pager_->getPage(page_id);

    BPlusTreePage bpt_page(page);
    bpt_page.initialize_page();
    bpt_page.set_leaf(false);
    bpt_page.set_key_type(key_type_);
    release_node(page_id, true);
    node_count_++;
    return page_id;
}

} // namespace engine
} // namespace minidb