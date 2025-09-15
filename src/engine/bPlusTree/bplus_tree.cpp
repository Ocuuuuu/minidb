#include "engine/bPlusTree/bplus_tree_page.h"
#include "storage/Pager.h"
#include "engine/bPlusTree/bplus_tree.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <cassert>

namespace minidb {
namespace engine {

BPlusTree::BPlusTree(std::shared_ptr<storage::Pager> pager,
                     PageID root_page_id,
                     TypeId key_type)
    : pager_(pager), root_page_id_(root_page_id), key_type_(key_type) {}

bool BPlusTree::insert(const Value& key, const RID& rid) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        // 如果树为空，创建第一个叶子节点作为根节点
        root_page_id_ = create_new_leaf_node();

        storage::Page* page = pager_->getPage(root_page_id_);
        BPlusTreePage root_node(page);
        bool success = root_node.insert_leaf_pair(key, rid);
        pager_->releasePage(root_page_id_, success);
        return success;
    }

    Value promoted_key;
    PageID new_child_page_id = INVALID_PAGE_ID;
    bool need_split = false;

    // 递归插入
    bool result = insert_recursive(root_page_id_, key, rid,
                                 &promoted_key, &new_child_page_id, &need_split);

    if (need_split) {
        // 根节点需要分裂，创建新的根节点
        result = create_new_root(root_page_id_, promoted_key, new_child_page_id);
    }

    if (!result) {
        std::cerr << "Insert failed for key: " << key.toString() << std::endl;
    }
    return result;
}

RID BPlusTree::search(const Value& key) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return RID::invalid();
    }

    try {
        PageID leaf_page_id = find_leaf_page(key);
        storage::Page* page = pager_->getPage(leaf_page_id);
        BPlusTreePage node(page);

        int index = node.find_key_index(key);
        if (index >= 0) {
            RID result = node.get_rid_at(index);
            pager_->releasePage(leaf_page_id, false);
            return result;
        }

        pager_->releasePage(leaf_page_id, false);
        return RID::invalid();
    } catch (const std::exception& e) {
        std::cerr << "Search error: " << e.what() << std::endl;
        return RID::invalid();
    }
}

std::vector<RID> BPlusTree::range_search(const Value& begin, const Value& end) {
    std::vector<RID> results;
    // TODO: 实现真正的范围扫描
    return results;
}

bool BPlusTree::remove(const Value& key) {
    // TODO: 实现删除操作
    return false;
}

uint32_t BPlusTree::get_height() const {
    // TODO: 实现高度计算
    return 1;
}

uint32_t BPlusTree::get_node_count() const {
    // TODO: 实现节点计数
    return 1;
}

BPlusTreePage BPlusTree::get_node(PageID page_id) {
    storage::Page* page = pager_->getPage(page_id);
    return BPlusTreePage(page);
}

void BPlusTree::release_node(PageID page_id, bool is_dirty) {
    pager_->releasePage(page_id, is_dirty);
}

PageID BPlusTree::find_leaf_page(const Value& key) {
    PageID current_page_id = root_page_id_;

    while (true) {
        storage::Page* page = pager_->getPage(current_page_id);
        BPlusTreePage node(page);

        if (node.is_leaf()) {
            pager_->releasePage(current_page_id, false);
            return current_page_id;
        }

        int index = node.find_key_index(key);
        if (index < 0) {
            index = -index - 1;
        }

        PageID next_page_id = node.get_child_page_id_at(index);
        pager_->releasePage(current_page_id, false);
        current_page_id = next_page_id;
    }
}

bool BPlusTree::insert_recursive(PageID current_page_id, const Value& key,
                                const RID& rid, Value* promoted_key,
                                PageID* new_child_page_id, bool* need_split) {
    storage::Page* page = pager_->getPage(current_page_id);
    BPlusTreePage node(page);

    if (node.is_leaf()) {
        // 叶子节点处理
        if (!node.is_full()) {
            // 节点未满，直接插入
            bool success = node.insert_leaf_pair(key, rid);
            pager_->releasePage(current_page_id, success);
            *need_split = false;
            return success;
        } else {
            // 节点已满，需要分裂
            bool success = split_leaf_node(current_page_id, promoted_key, new_child_page_id);
            pager_->releasePage(current_page_id, success);

            if (success) {
                *need_split = true;

                // 确定新键应该插入到哪个节点
                if (key < *promoted_key) {
                    // 插入到原节点
                    storage::Page* old_page = pager_->getPage(current_page_id);
                    BPlusTreePage old_node(old_page);
                    success = old_node.insert_leaf_pair(key, rid);
                    pager_->releasePage(current_page_id, success);
                } else {
                    // 插入到新节点
                    storage::Page* new_page = pager_->getPage(*new_child_page_id);
                    BPlusTreePage new_node(new_page);
                    success = new_node.insert_leaf_pair(key, rid);
                    pager_->releasePage(*new_child_page_id, success);
                }
            }
            return success;
        }
    } else {
        // 内部节点处理
        int index = node.find_key_index(key);
        if (index < 0) {
            index = -index - 1;
        }
        PageID child_page_id = node.get_child_page_id_at(index);

        // 递归插入到子节点
        Value child_promoted_key;
        PageID child_new_page_id = INVALID_PAGE_ID;
        bool child_need_split = false;

        bool success = insert_recursive(child_page_id, key, rid,
                                      &child_promoted_key, &child_new_page_id,
                                      &child_need_split);

        if (!success) {
            pager_->releasePage(current_page_id, false);
            return false;
        }

        if (child_need_split) {
            // 子节点分裂了，需要在当前节点插入提升的键
            if (!node.is_full()) {
                // 当前节点有空间，直接插入
                success = node.insert_internal_pair(child_promoted_key, child_new_page_id);
                pager_->releasePage(current_page_id, success);
                *need_split = false;
                return success;
            } else {
                // 当前节点也满了，需要分裂
                success = split_internal_node(current_page_id, promoted_key, new_child_page_id);
                pager_->releasePage(current_page_id, success);

                if (success) {
                    *need_split = true;

                    // 确定提升的键应该插入到哪个节点
                    if (child_promoted_key < *promoted_key) {
                        // 插入到原节点
                        storage::Page* old_page = pager_->getPage(current_page_id);
                        BPlusTreePage old_node(old_page);
                        success = old_node.insert_internal_pair(child_promoted_key, child_new_page_id);
                        pager_->releasePage(current_page_id, success);
                    } else {
                        // 插入到新节点
                        storage::Page* new_page = pager_->getPage(*new_child_page_id);
                        BPlusTreePage new_node(new_page);
                        success = new_node.insert_internal_pair(child_promoted_key, child_new_page_id);
                        pager_->releasePage(*new_child_page_id, success);
                    }
                }
                return success;
            }
        } else {
            // 子节点没有分裂，直接返回
            pager_->releasePage(current_page_id, false);
            *need_split = false;
            return true;
        }
    }
}

bool BPlusTree::insert_into_leaf(PageID leaf_page_id, const Value& key, const RID& rid) {
    storage::Page* page = pager_->getPage(leaf_page_id);
    BPlusTreePage node(page);
    bool success = node.insert_leaf_pair(key, rid);
    pager_->releasePage(leaf_page_id, success);
    return success;
}

bool BPlusTree::split_leaf_node(PageID leaf_page_id, Value* promoted_key, PageID* new_page_id) {
    storage::Page* old_page = pager_->getPage(leaf_page_id);
    BPlusTreePage old_node(old_page);

    if (!old_node.is_leaf()) {
        pager_->releasePage(leaf_page_id, false);
        return false;
    }

    // 创建新叶子节点
    *new_page_id = create_new_leaf_node();
    storage::Page* new_page = pager_->getPage(*new_page_id);
    BPlusTreePage new_node(new_page);

    // 设置链表指针
    new_node.set_next_page_id(old_node.get_next_page_id());
    old_node.set_next_page_id(*new_page_id);

    // 计算分裂点
    int total_keys = old_node.get_key_count();
    int split_index = total_keys / 2;

    // 获取提升的键（分裂后的第一个键）
    *promoted_key = old_node.get_key_at(split_index);

    // 将后半部分数据移动到新节点
    for (int i = split_index; i < total_keys; ++i) {
        Value key = old_node.get_key_at(i);
        RID rid = old_node.get_rid_at(i);
        new_node.insert_leaf_pair(key, rid);
    }

    // 更新原节点的键数量
    old_node.set_key_count(split_index);

    pager_->releasePage(leaf_page_id, true);
    pager_->releasePage(*new_page_id, true);
    return true;
}

bool BPlusTree::split_internal_node(PageID internal_page_id, Value* promoted_key, PageID* new_page_id) {
    storage::Page* old_page = pager_->getPage(internal_page_id);
    BPlusTreePage old_node(old_page);

    if (old_node.is_leaf()) {
        pager_->releasePage(internal_page_id, false);
        return false;
    }

    // 创建新内部节点
    *new_page_id = create_new_internal_node();
    storage::Page* new_page = pager_->getPage(*new_page_id);
    BPlusTreePage new_node(new_page);

    // 计算分裂点
    int total_keys = old_node.get_key_count();
    int split_index = total_keys / 2;

    // 获取提升的键
    *promoted_key = old_node.get_key_at(split_index);

    // 将后半部分键和指针移动到新节点
    // 注意：内部节点有n+1个指针
    for (int i = split_index + 1; i < total_keys; ++i) {
        Value key = old_node.get_key_at(i);
        new_node.insert_internal_pair(key, old_node.get_child_page_id_at(i + 1));
    }

    // 设置新节点的第一个子指针
    new_node.set_child_page_id_at(0, old_node.get_child_page_id_at(split_index + 1));

    // 更新原节点的键数量
    old_node.set_key_count(split_index);

    pager_->releasePage(internal_page_id, true);
    pager_->releasePage(*new_page_id, true);
    return true;
}

bool BPlusTree::create_new_root(PageID left_child_id, const Value& key, PageID right_child_id) {
    // 创建新的根节点
    PageID new_root_id = create_new_internal_node();
    storage::Page* root_page = pager_->getPage(new_root_id);
    BPlusTreePage root_node(root_page);

    // 设置根节点的第一个子指针
    root_node.set_child_page_id_at(0, left_child_id);

    // 插入分隔键和右子指针
    bool success = root_node.insert_internal_pair(key, right_child_id);

    if (success) {
        // 更新根页面ID
        root_page_id_ = new_root_id;
    }

    pager_->releasePage(new_root_id, success);
    return success;
}

PageID BPlusTree::create_new_leaf_node() {
    PageID new_page_id = pager_->allocatePage();
    storage::Page* page = pager_->getPage(new_page_id);

    char* page_data = page->getData();
    BPlusNodeHeader header;
    header.is_leaf = 1;
    header.key_count = 0;
    header.next_page_id = INVALID_PAGE_ID;
    std::memcpy(page_data, &header, sizeof(BPlusNodeHeader));

    std::memset(page_data + sizeof(BPlusNodeHeader), 0,
               PAGE_SIZE - sizeof(BPlusNodeHeader));

    page->setDirty(true);
    pager_->releasePage(new_page_id, true);
    return new_page_id;
}

PageID BPlusTree::create_new_internal_node() {
    PageID new_page_id = pager_->allocatePage();
    storage::Page* page = pager_->getPage(new_page_id);

    char* page_data = page->getData();
    BPlusNodeHeader header;
    header.is_leaf = 0;
    header.key_count = 0;
    header.next_page_id = INVALID_PAGE_ID;
    std::memcpy(page_data, &header, sizeof(BPlusNodeHeader));

    std::memset(page_data + sizeof(BPlusNodeHeader), 0,
               PAGE_SIZE - sizeof(BPlusNodeHeader));

    page->setDirty(true);
    pager_->releasePage(new_page_id, true);
    return new_page_id;
}

} // namespace engine
} // namespace minidb