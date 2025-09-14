#include "engine/bPlusTree/bplus_tree_page.h"
#include "storage/Pager.h"
#include "engine/bPlusTree/bplus_tree.h"
#include <iostream>
#include <cstring>    // 用于 memcpy, memset
#include <vector>     // 用于 std::vector

namespace minidb {
namespace engine {

// B+树构造函数
// 参数: pager - 页面管理器指针
//       root_page_id - 根页面ID
//       key_type - 键值类型
BPlusTree::BPlusTree(std::shared_ptr<storage::Pager> pager, 
                     PageID root_page_id, 
                     TypeId key_type)
    : pager_(pager), root_page_id_(root_page_id), key_type_(key_type) {}

// 插入键值对到B+树
// 参数: key - 要插入的键值
//       rid - 对应的记录ID
// 返回: 插入成功返回true，失败返回false
bool BPlusTree::insert(const Value& key, const RID& rid) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        // 如果树为空，创建第一个叶子节点作为根节点
        root_page_id_ = create_new_leaf_node();
    }
    
    // 递归插入键值对
    bool result = insert_recursive(root_page_id_, key, rid);
    if (!result) {
        std::cerr << "Insert failed for key: " << key.toString() << std::endl;
    }
    return result;
}

// 在B+树中搜索指定键值
// 参数: key - 要搜索的键值
// 返回: 对应的记录ID，如果未找到返回无效RID
RID BPlusTree::search(const Value& key) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return RID::invalid(); // 空树返回无效记录
    }
    
    try {
        // 找到包含该键的叶子页面
        PageID leaf_page_id = find_leaf_page(key);
        storage::Page* page = pager_->getPage(leaf_page_id);
        BPlusTreePage node(page);
        
        // 在叶子节点中查找键的位置
        int index = node.find_key_index(key);
        if (index >= 0) {
            // 找到键，返回对应的记录ID
            RID result = node.get_rid_at(index);
            pager_->releasePage(leaf_page_id, false);
            return result;
        }
        
        // 未找到键，返回无效记录
        pager_->releasePage(leaf_page_id, false);
        return RID::invalid();
    } catch (const std::exception& e) {
        std::cerr << "Search error: " << e.what() << std::endl;
        return RID::invalid();
    }
}

// 范围搜索，查找在begin和end之间的所有键值对应的记录
// 参数: begin - 范围起始键值
//       end - 范围结束键值
// 返回: 范围内所有记录ID的向量
std::vector<RID> BPlusTree::range_search(const Value& begin, const Value& end) {
    std::vector<RID> results;
    if (root_page_id_ == INVALID_PAGE_ID) {
        return results; // 空树返回空结果
    }
    
    // 简化实现：遍历所有叶子节点
    // TODO: 实现真正的范围扫描
    return results;
}

// 从B+树中删除指定键值
// 参数: key - 要删除的键值
// 返回: 删除成功返回true，失败返回false
bool BPlusTree::remove(const Value& key) {
    // TODO: 实现删除操作
    return false;
}

// 获取B+树的高度
// 返回: 树的高度
uint32_t BPlusTree::get_height() const {
    // TODO: 实现高度计算
    return 1;
}

// 获取B+树中节点的数量
// 返回: 节点数量
uint32_t BPlusTree::get_node_count() const {
    // TODO: 实现节点计数
    return 1;
}

// 获取指定页面ID的B+树节点
// 参数: page_id - 页面ID
// 返回: B+树节点对象
BPlusTreePage BPlusTree::get_node(PageID page_id) {
    storage::Page* page = pager_->getPage(page_id);
    return BPlusTreePage(page);
}

// 释放B+树节点
// 参数: page_id - 页面ID
//       is_dirty - 标记页面是否被修改
void BPlusTree::release_node(PageID page_id, bool is_dirty) {
    pager_->releasePage(page_id, is_dirty);
}

// 查找包含指定键的叶子页面
// 参数: key - 要查找的键值
// 返回: 叶子页面的ID
PageID BPlusTree::find_leaf_page(const Value& key) {
    PageID current_page_id = root_page_id_;
    
    while (true) {
        storage::Page* page = pager_->getPage(current_page_id);
        BPlusTreePage node(page);
        
        if (node.is_leaf()) {
            // 找到叶子节点，返回页面ID
            pager_->releasePage(current_page_id, false);
            return current_page_id;
        }
        
        // 在内部节点中查找下一个子节点
        int index = node.find_key_index(key);
        if (index < 0) {
            index = -index - 1; // 处理未找到键的情况
        }
        
        // 获取子页面ID并继续查找
        PageID next_page_id = node.get_child_page_id_at(index);
        pager_->releasePage(current_page_id, false);
        current_page_id = next_page_id;
    }
}

// 递归插入键值对到B+树
// 参数: current_page_id - 当前处理的页面ID
//       key - 要插入的键值
//       rid - 对应的记录ID
// 返回: 插入成功返回true，失败返回false
bool BPlusTree::insert_recursive(PageID current_page_id, const Value& key, const RID& rid) {
    storage::Page* page = pager_->getPage(current_page_id);
    BPlusTreePage node(page);
    
    if (node.is_leaf()) {
        // 处理叶子节点插入
        if (!node.is_full()) {
            // 节点未满，直接插入
            bool success = node.insert_leaf_pair(key, rid);
            pager_->releasePage(current_page_id, success);
            return success;
        } else {
            // 节点已满，需要分裂
            bool success = split_leaf_node(current_page_id);
            pager_->releasePage(current_page_id, success);
            if (success) {
                // 分裂后重新尝试插入
                return insert(key, rid);
            }
            return false;
        }
    } else {
        // 内部节点：找到合适的子节点继续递归
        int index = node.find_key_index(key);
        if (index < 0) {
            index = -index - 1; // 处理未找到键的情况
        }
        PageID child_page_id = node.get_child_page_id_at(index);
        pager_->releasePage(current_page_id, false);
        
        // 递归插入到子节点
        return insert_recursive(child_page_id, key, rid);
    }
}

// 直接插入键值对到叶子节点
// 参数: leaf_page_id - 叶子页面ID
//       key - 要插入的键值
//       rid - 对应的记录ID
// 返回: 插入成功返回true，失败返回false
bool BPlusTree::insert_into_leaf(PageID leaf_page_id, const Value& key, const RID& rid) {
    storage::Page* page = pager_->getPage(leaf_page_id);
    BPlusTreePage node(page);
    bool success = node.insert_leaf_pair(key, rid);
    pager_->releasePage(leaf_page_id, success);
    return success;
}

// 分裂叶子节点
// 参数: leaf_page_id - 要分裂的叶子页面ID
// 返回: 分裂成功返回true，失败返回false
bool BPlusTree::split_leaf_node(PageID leaf_page_id) {
    // 简易分裂：创建新节点，平均分配数据
    storage::Page* old_page = pager_->getPage(leaf_page_id);
    BPlusTreePage old_node(old_page);
    
    if (!old_node.is_leaf()) {
        pager_->releasePage(leaf_page_id, false);
        return false; // 非叶子节点不能按叶子节点方式分裂
    }
    
    // 创建新叶子节点
    PageID new_page_id = create_new_leaf_node();
    storage::Page* new_page = pager_->getPage(new_page_id);
    BPlusTreePage new_node(new_page);
    
    // 设置链表指针，维护叶子节点链表
    new_node.set_next_page_id(old_node.get_next_page_id());
    old_node.set_next_page_id(new_page_id);
    
    // 平均分配数据（简化：移动后半部分数据）
    int total_keys = old_node.get_key_count();
    int split_index = total_keys / 2;
    
    // 将后半部分数据移动到新节点
    for (int i = split_index; i < total_keys; ++i) {
        Value key = old_node.get_key_at(i);
        RID rid = old_node.get_rid_at(i);
        new_node.insert_leaf_pair(key, rid);
    }
    
    // 更新原节点的键数量
    old_node.set_key_count(split_index);
    
    // 标记页面为脏页并释放
    pager_->releasePage(leaf_page_id, true);
    pager_->releasePage(new_page_id, true);
    
    // TODO: 需要更新父节点
    return true;
}

// 分裂内部节点
// 参数: internal_page_id - 要分裂的内部页面ID
// 返回: 分裂成功返回true，失败返回false
bool BPlusTree::split_internal_node(PageID internal_page_id) {
    // TODO: 实现内部节点分裂
    return false;
}

// 创建新的叶子节点
// 返回: 新创建的叶子页面ID
PageID BPlusTree::create_new_leaf_node() {
    PageID new_page_id = pager_->allocatePage();
    storage::Page* page = pager_->getPage(new_page_id);
    
    // 初始化节点数据
    char* page_data = page->getData();
    BPlusNodeHeader header;
    header.is_leaf = 1;        // 标记为叶子节点
    header.key_count = 0;      // 初始键数量为0
    header.next_page_id = INVALID_PAGE_ID; // 下一个页面ID初始为无效
    std::memcpy(page_data, &header, sizeof(BPlusNodeHeader));
    
    // 清空剩余数据区域
    std::memset(page_data + sizeof(BPlusNodeHeader), 0, 
               PAGE_SIZE - sizeof(BPlusNodeHeader));
    
    // 标记页面为脏页并释放
    page->setDirty(true);
    pager_->releasePage(new_page_id, true);
    return new_page_id;
}

// 创建新的内部节点
// 返回: 新创建的内部页面ID
PageID BPlusTree::create_new_internal_node() {
    PageID new_page_id = pager_->allocatePage();
    storage::Page* page = pager_->getPage(new_page_id);
    
    // 初始化节点数据
    char* page_data = page->getData();
    BPlusNodeHeader header;
    header.is_leaf = 0;        // 标记为内部节点
    header.key_count = 0;      // 初始键数量为0
    header.next_page_id = INVALID_PAGE_ID; // 下一个页面ID初始为无效
    std::memcpy(page_data, &header, sizeof(BPlusNodeHeader));
    
    // 清空剩余数据区域
    std::memset(page_data + sizeof(BPlusNodeHeader), 0, 
               PAGE_SIZE - sizeof(BPlusNodeHeader));
    
    // 标记页面为脏页并释放
    page->setDirty(true);
    pager_->releasePage(new_page_id, true);
    return new_page_id;
}

} // namespace engine
} // namespace minidb