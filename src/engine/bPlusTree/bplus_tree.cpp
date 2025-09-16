#include "engine/bPlusTree/bplus_tree_page.h"
#include "storage/Pager.h"
#include "engine/bPlusTree/bplus_tree.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <queue>
#include <memory>
#include <limits>

namespace minidb {
namespace engine {

BPlusTree::BPlusTree(std::shared_ptr<storage::Pager> pager,
                     PageID root_page_id,
                     TypeId key_type)
    : pager_(pager), root_page_id_(root_page_id), key_type_(key_type)
{
    // 如果根页面有效，初始化节点计数
    if (root_page_id_ != INVALID_PAGE_ID) {
        node_count_ = 1;
    }
}

Value BPlusTree::get_min_value_for_type(TypeId type) const {
    switch (type) {
        case TypeId::BOOLEAN:
            return Value(false);
        case TypeId::INTEGER:
            return Value(std::numeric_limits<int32_t>::min());
        case TypeId::VARCHAR:
            return Value("");
        default:
            return Value();
    }
}

void BPlusTree::validate_key_type(const Value& key) const {
    if (key.getType() != key_type_) {
        throw std::runtime_error("Key type mismatch in B+ tree operation");
    }
}

bool BPlusTree::insert(const Value& key, const RID& rid) {
    validate_key_type(key);

    if (root_page_id_ == INVALID_PAGE_ID) {
        root_page_id_ = create_new_leaf_node();
        auto node = get_node(root_page_id_);
        bool success = node->insert_leaf_pair(key, rid);
        release_node(root_page_id_, success);
        return success;
    }

    Value promoted_key;
    PageID new_child_page_id = INVALID_PAGE_ID;
    bool need_split = false;

    bool result = insert_recursive(root_page_id_, key, rid,
                                 &promoted_key, &new_child_page_id, &need_split);

    if (need_split) {
        result = create_new_root(root_page_id_, promoted_key, new_child_page_id);
    }

    return result;
}

RID BPlusTree::search(const Value& key) const {
    validate_key_type(key);

    if (root_page_id_ == INVALID_PAGE_ID) {
        return RID::invalid();
    }

    try {
        PageID leaf_page_id = find_leaf_page(key);
        auto node = get_node(leaf_page_id);

        int index = node->find_key_index(key);
        if (index >= 0) {
            RID result = node->get_rid_at(index);
            release_node(leaf_page_id, false);
            return result;
        }

        release_node(leaf_page_id, false);
        return RID::invalid();
    } catch (const std::exception& e) {
        std::cerr << "Search error: " << e.what() << std::endl;
        return RID::invalid();
    }
}

std::vector<RID> BPlusTree::range_search(const Value& begin, const Value& end) const {
    validate_key_type(begin);
    validate_key_type(end);

    std::vector<RID> results;

    if (root_page_id_ == INVALID_PAGE_ID) {
        return results;
    }

    try {
        PageID current_page_id = find_leaf_page(begin);

        while (current_page_id != INVALID_PAGE_ID) {
            auto node = get_node(current_page_id);

            int start_index = 0;
            int pos = node->find_key_index(begin);
            start_index = (pos >= 0) ? pos : (-pos - 1);

            for (int i = start_index; i < node->get_key_count(); ++i) {
                Value current_key = node->get_key_at(i);
                if (current_key > end) {
                    release_node(current_page_id, false);
                    return results;
                }
                if (current_key > begin && current_key < end) {
                    results.push_back(node->get_rid_at(i));
                }
            }

            PageID next_page = node->get_next_page_id();
            release_node(current_page_id, false);
            current_page_id = next_page;
        }
    } catch (const std::exception& e) {
        std::cerr << "Range search error: " << e.what() << std::endl;
    }

    return results;
}

bool BPlusTree::remove(const Value& key) {
    validate_key_type(key);

    if (root_page_id_ == INVALID_PAGE_ID) {
        return false;
    }

    bool need_merge = false;
    bool result = remove_recursive(root_page_id_, key, &need_merge);

    if (result && root_page_id_ != INVALID_PAGE_ID) {
        auto root_node = get_node(root_page_id_);
        if (root_node->get_key_count() == 0) {
            if (root_node->is_leaf()) {
                pager_->deallocatePage(root_page_id_);
                root_page_id_ = INVALID_PAGE_ID;
                node_count_--;
            } else {
                PageID new_root_id = root_node->get_child_page_id_at(0);
                pager_->deallocatePage(root_page_id_);
                root_page_id_ = new_root_id;
                node_count_--;
            }
        }
        release_node(root_page_id_, false);
    }

    return result;
}

bool BPlusTree::handle_underflow(PageID parent_page_id, PageID page_id, int index) {
    std::cout << "Underflow handling called for:" << std::endl;
    std::cout << "  Parent page: " << parent_page_id << std::endl;
    std::cout << "  Child page: " << page_id << std::endl;
    std::cout << "  Index: " << index << std::endl;

    // 简化实现：暂时不做真正的下溢处理
    return true;
}

uint32_t BPlusTree::get_height() const {
    if (root_page_id_ == INVALID_PAGE_ID) return 0;

    uint32_t height = 0;
    PageID current_page_id = root_page_id_;

    try {
        while (current_page_id != INVALID_PAGE_ID) {
            auto node = get_node(current_page_id);
            height++;
            if (node->is_leaf()) {
                release_node(current_page_id, false);
                break;
            }
            PageID first_child = node->get_child_page_id_at(0);
            release_node(current_page_id, false);
            current_page_id = first_child;
        }
    } catch (const std::exception& e) {
        std::cerr << "Get height error: " << e.what() << std::endl;
    }

    return height;
}

uint32_t BPlusTree::get_node_count() const {
    return node_count_;
}

void BPlusTree::print_tree() const {
    if (root_page_id_ == INVALID_PAGE_ID) {
        std::cout << "Tree is empty" << std::endl;
        return;
    }

    std::queue<std::pair<PageID, int>> queue;
    queue.push({root_page_id_, 0});

    try {
        while (!queue.empty()) {
            auto [current_id, level] = queue.front();
            queue.pop();

            auto node = get_node(current_id);
            std::cout << "Level " << level << " - Page " << current_id
                      << " (" << (node->is_leaf() ? "Leaf" : "Internal") << "): ";

            for (int i = 0; i < node->get_key_count(); ++i) {
                std::cout << node->get_key_at(i).toString() << " ";
            }
            std::cout << std::endl;

            if (!node->is_leaf()) {
                for (int i = 0; i <= node->get_key_count(); ++i) {
                    PageID child_id = node->get_child_page_id_at(i);
                    if (child_id != INVALID_PAGE_ID) {
                        queue.push({child_id, level + 1});
                    }
                }
            }
            release_node(current_id, false);
        }
    } catch (const std::exception& e) {
        std::cerr << "Print tree error: " << e.what() << std::endl;
    }
}

std::unique_ptr<BPlusTreePage> BPlusTree::get_node(PageID page_id) const {
    storage::Page* page = pager_->getPage(page_id);
    if (!page) {
        throw std::runtime_error("Failed to get page: " + std::to_string(page_id));
    }
    return std::make_unique<BPlusTreePage>(page);
}

void BPlusTree::release_node(PageID page_id, bool is_dirty) const {
    pager_->releasePage(page_id, is_dirty);
}

PageID BPlusTree::find_leaf_page(const Value& key) const {
    PageID current_page_id = root_page_id_;

    while (current_page_id != INVALID_PAGE_ID) {
        auto node = get_node(current_page_id);
        if (node->is_leaf()) {
            release_node(current_page_id, false);
            return current_page_id;
        }

        int index = node->find_key_index(key);
        index = (index < 0) ? -index - 1 : index;
        PageID next_page_id = node->get_child_page_id_at(index);
        release_node(current_page_id, false);
        current_page_id = next_page_id;
    }

    return INVALID_PAGE_ID;
}

bool BPlusTree::insert_recursive(PageID current_page_id, const Value& key,
                                const RID& rid, Value* promoted_key,
                                PageID* new_child_page_id, bool* need_split) {
    auto node = get_node(current_page_id);
    bool success = true;
    bool is_dirty = false;

    if (node->is_leaf()) {
        if (!node->is_full()) {
            success = node->insert_leaf_pair(key, rid);
            is_dirty = success;
            *need_split = false;
        } else {
            // 先不释放当前节点，分裂函数需要它
            success = split_leaf_node(current_page_id, key, rid, promoted_key, new_child_page_id);
            *need_split = true;
            return success;
        }
    } else {
        int index = node->find_key_index(key);
        index = (index < 0) ? -index - 1 : index;
        PageID child_page_id = node->get_child_page_id_at(index);

        Value child_promoted_key;
        PageID child_new_page_id = INVALID_PAGE_ID;
        bool child_need_split = false;

        success = insert_recursive(child_page_id, key, rid,
                                  &child_promoted_key, &child_new_page_id,
                                  &child_need_split);

        if (success && child_need_split) {
            if (!node->is_full()) {
                success = node->insert_internal_pair(child_promoted_key, child_new_page_id);
                is_dirty = success;
                *need_split = false;
            } else {
                // 先不释放当前节点，分裂函数需要它
                success = split_internal_node(current_page_id, child_promoted_key, child_new_page_id,
                                            promoted_key, new_child_page_id);
                *need_split = true;
                return success;
            }
        } else {
            *need_split = false;
        }
    }

    release_node(current_page_id, is_dirty);
    return success;
}

bool BPlusTree::split_leaf_node(PageID leaf_page_id, const Value& new_key, const RID& new_rid,
                               Value* promoted_key, PageID* new_page_id) {
    auto old_node = get_node(leaf_page_id);
    if (!old_node->is_leaf()) {
        release_node(leaf_page_id, false);
        return false;
    }

    *new_page_id = create_new_leaf_node();
    auto new_node = get_node(*new_page_id);

    new_node->set_next_page_id(old_node->get_next_page_id());
    old_node->set_next_page_id(*new_page_id);

    int total_keys = old_node->get_key_count();
    int split_index = total_keys / 2;

    // 确保分裂索引有效
    if (split_index <= 0 || split_index >= total_keys) {
        release_node(leaf_page_id, false);
        release_node(*new_page_id, false);
        return false;
    }

    *promoted_key = old_node->get_key_at(split_index);

    // 移动后半部分键值对到新节点
    for (int i = split_index; i < total_keys; ++i) {
        Value key = old_node->get_key_at(i);
        RID rid = old_node->get_rid_at(i);
        new_node->insert_leaf_pair(key, rid);
    }

    // 更新原节点的键数量
    old_node->set_key_count(split_index);

    // 插入新键值对到合适的节点
    PageID target_page = (new_key < *promoted_key) ? leaf_page_id : *new_page_id;
    auto target_node = get_node(target_page);
    bool insert_success = target_node->insert_leaf_pair(new_key, new_rid);

    release_node(leaf_page_id, true);
    release_node(*new_page_id, true);
    release_node(target_page, insert_success);

    return insert_success;
}

bool BPlusTree::split_internal_node(PageID internal_page_id, const Value& new_key, PageID new_child_id,
                                  Value* promoted_key, PageID* new_page_id) {
    auto old_node = get_node(internal_page_id);
    if (old_node->is_leaf()) {
        release_node(internal_page_id, false);
        return false;
    }

    *new_page_id = create_new_internal_node();
    auto new_node = get_node(*new_page_id);

    int total_keys = old_node->get_key_count();
    int split_index = total_keys / 2;

    // 确保分裂索引有效
    if (split_index <= 0 || split_index >= total_keys) {
        release_node(internal_page_id, false);
        release_node(*new_page_id, false);
        return false;
    }

    *promoted_key = old_node->get_key_at(split_index);

    // 移动后半部分键和子指针到新节点
    for (int i = split_index + 1; i < total_keys; ++i) {
        Value key = old_node->get_key_at(i);
        PageID child_id = old_node->get_child_page_id_at(i + 1);
        new_node->insert_internal_pair(key, child_id);
    }

    // 设置新节点的第一个子指针
    PageID first_child_id = old_node->get_child_page_id_at(split_index + 1);
    new_node->set_child_page_id_at(0, first_child_id);

    // 更新原节点的键数量
    old_node->set_key_count(split_index);

    // 插入新键值对到合适的节点
    PageID target_page = (new_key < *promoted_key) ? internal_page_id : *new_page_id;
    auto target_node = get_node(target_page);
    bool insert_success = target_node->insert_internal_pair(new_key, new_child_id);

    release_node(internal_page_id, true);
    release_node(*new_page_id, true);
    release_node(target_page, insert_success);

    return insert_success;
}

bool BPlusTree::create_new_root(PageID left_child_id, const Value& key, PageID right_child_id) {
    PageID new_root_id = create_new_internal_node();
    auto root_node = get_node(new_root_id);

    root_node->set_child_page_id_at(0, left_child_id);
    bool success = root_node->insert_internal_pair(key, right_child_id);

    if (success) {
        root_page_id_ = new_root_id;
    }

    release_node(new_root_id, success);
    return success;
}

bool BPlusTree::remove_from_leaf(PageID leaf_page_id, const Value& key) {
    auto node = get_node(leaf_page_id);
    int index = node->find_key_index(key);
    if (index < 0) {
        release_node(leaf_page_id, false);
        return false;
    }

    bool success = node->remove_leaf_pair(index);
    release_node(leaf_page_id, success);
    return success;
}

bool BPlusTree::remove_recursive(PageID current_page_id, const Value& key, bool* need_merge) {
    auto node = get_node(current_page_id);
    bool success = true;
    bool is_dirty = false;
    *need_merge = false;

    if (node->is_leaf()) {
        int index = node->find_key_index(key);
        if (index < 0) {
            release_node(current_page_id, false);
            return false;
        }

        success = node->remove_leaf_pair(index);
        is_dirty = success;
        *need_merge = node->is_underflow();
    } else {
        int index = node->find_key_index(key);
        index = (index < 0) ? -index - 1 : index;
        PageID child_page_id = node->get_child_page_id_at(index);

        bool child_need_merge = false;
        success = remove_recursive(child_page_id, key, &child_need_merge);

        if (success && child_need_merge) {
            success = handle_underflow(current_page_id, child_page_id, index);
            is_dirty = success;
            *need_merge = node->is_underflow();
        }
    }

    release_node(current_page_id, is_dirty);
    return success;
}

PageID BPlusTree::create_new_leaf_node() {
    PageID new_page_id = pager_->allocatePage();
    node_count_++;

    storage::Page* page = pager_->getPage(new_page_id);
    char* data = page->getData();

    BPlusNodeHeader header;
    header.is_leaf = 1;
    header.key_count = 0;
    header.next_page_id = INVALID_PAGE_ID;
    header.key_type = key_type_;
    header.key_size = calculate_key_size(key_type_);

    std::memcpy(data, &header, sizeof(BPlusNodeHeader));
    page->setDirty(true);

    pager_->releasePage(new_page_id, true);
    return new_page_id;
}

uint16_t BPlusTree::calculate_key_size(TypeId type) const {
    switch (type) {
    case TypeId::BOOLEAN: return sizeof(bool);
    case TypeId::INTEGER: return sizeof(int32_t);
    case TypeId::VARCHAR: return 0;
    default: return sizeof(int32_t);
    }
}

PageID BPlusTree::create_new_internal_node() {
    PageID new_page_id = pager_->allocatePage();
    node_count_++;

    storage::Page* page = pager_->getPage(new_page_id);
    char* data = page->getData();

    BPlusNodeHeader header;
    header.is_leaf = 0;
    header.key_count = 0;
    header.next_page_id = INVALID_PAGE_ID;
    header.key_type = key_type_;
    header.key_size = calculate_key_size(key_type_);

    std::memcpy(data, &header, sizeof(BPlusNodeHeader));

    // 初始化所有子指针为 INVALID_PAGE_ID
    PageID* children = reinterpret_cast<PageID*>(data + sizeof(BPlusNodeHeader));
    for (int i = 0; i <= INTERNAL_NODE_MAX_KEYS; i++) {
        children[i] = INVALID_PAGE_ID;
    }

    page->setDirty(true);
    pager_->releasePage(new_page_id, true);
    return new_page_id;
}

PageID BPlusTree::find_first_leaf_page() const {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return INVALID_PAGE_ID;
    }

    PageID current_page_id = root_page_id_;

    try {
        while (current_page_id != INVALID_PAGE_ID) {
            auto node = get_node(current_page_id);
            if (node->is_leaf()) {
                release_node(current_page_id, false);
                return current_page_id;
            }
            PageID first_child = node->get_child_page_id_at(0);
            release_node(current_page_id, false);
            current_page_id = first_child;
        }
    } catch (const std::exception& e) {
        std::cerr << "Find first leaf error: " << e.what() << std::endl;
    }

    return INVALID_PAGE_ID;
}

} // namespace engine
} // namespace minidb