#include "engine/bPlusTree/bplus_tree_page.h"
#include "storage/Page.h"
#include "common/Value.h"
#include <cstring>
#include <stdexcept>

namespace minidb {
namespace engine {

/**
 * @brief 构造函数，基于物理页创建B+树节点视图
 * @param page 指向物理页的指针，该页包含B+树节点数据
 */
BPlusTreePage::BPlusTreePage(storage::Page* page) : page_(page) {
    // 从页数据区域的前sizeof(BPlusNodeHeader)字节反序列化头部信息
    const char* data = page_->getData();
    std::memcpy(&header_, data, sizeof(BPlusNodeHeader));
}

/**
 * @brief 获取指定索引位置的键值
 * @param index 键的索引位置（0-based）
 * @return 对应位置的Value对象
 * @throw std::out_of_range 如果索引越界
 */
Value BPlusTreePage::get_key_at(int index) const {
    if (index < 0 || index >= header_.key_count) {
        throw std::out_of_range("Key index out of range: " + std::to_string(index));
    }
    
    size_t offset = get_key_offset(index);
    const char* data = get_data_start();
    return deserialize_value(data + offset, TypeId::INTEGER);
}

/**
 * @brief 设置指定索引位置的键值
 * @param index 键的索引位置（0-based）
 * @param key 要设置的键值
 * @throw std::out_of_range 如果索引越界
 */
void BPlusTreePage::set_key_at(int index, const Value& key) {
    if (index < 0 || index >= header_.key_count) {
        throw std::out_of_range("Key index out of range: " + std::to_string(index));
    }
    
    size_t offset = get_key_offset(index);
    char* data = get_data_start();
    serialize_value(key, data + offset);
    mark_dirty();
}

/**
 * @brief 获取叶子节点指定索引位置的记录ID(RID)
 * @param index RID的索引位置（0-based）
 * @return 对应位置的RID
 * @throw std::logic_error 如果不是叶子节点
 * @throw std::out_of_range 如果索引越界
 */
RID BPlusTreePage::get_rid_at(int index) const {
    if (!is_leaf()) {
        throw std::logic_error("Not a leaf node, cannot get RID");
    }
    if (index < 0 || index >= header_.key_count) {
        throw std::out_of_range("RID index out of range: " + std::to_string(index));
    }
    
    size_t offset = get_value_offset(index);
    const char* data = get_data_start();
    RID rid;
    std::memcpy(&rid, data + offset, sizeof(RID));
    return rid;
}

/**
 * @brief 设置叶子节点指定索引位置的记录ID(RID)
 * @param index RID的索引位置（0-based）
 * @param rid 要设置的RID值
 * @throw std::logic_error 如果不是叶子节点
 * @throw std::out_of_range 如果索引越界
 */
void BPlusTreePage::set_rid_at(int index, const RID& rid) {
    if (!is_leaf()) {
        throw std::logic_error("Not a leaf node, cannot set RID");
    }
    if (index < 0 || index >= header_.key_count) {
        throw std::out_of_range("RID index out of range: " + std::to_string(index));
    }
    
    size_t offset = get_value_offset(index);
    char* data = get_data_start();
    std::memcpy(data + offset, &rid, sizeof(RID));
    mark_dirty();
}

/**
 * @brief 获取内部节点指定索引位置的子页面ID
 * @param index 子指针的索引位置（0-based），内部节点有n+1个指针
 * @return 对应位置的子页面ID
 * @throw std::logic_error 如果是叶子节点
 * @throw std::out_of_range 如果索引越界
 */
PageID BPlusTreePage::get_child_page_id_at(int index) const {
    if (is_leaf()) {
        throw std::logic_error("Leaf node, cannot get child page ID");
    }
    if (index < 0 || index > header_.key_count) { // 内部节点有n+1个指针
        throw std::out_of_range("Child index out of range: " + std::to_string(index));
    }
    
    size_t offset = get_value_offset(index);
    const char* data = get_data_start();
    PageID page_id;
    std::memcpy(&page_id, data + offset, sizeof(PageID));
    return page_id;
}

/**
 * @brief 设置内部节点指定索引位置的子页面ID
 * @param index 子指针的索引位置（0-based）
 * @param page_id 要设置的子页面ID
 * @throw std::logic_error 如果是叶子节点
 * @throw std::out_of_range 如果索引越界
 */
void BPlusTreePage::set_child_page_id_at(int index, PageID page_id) {
    if (is_leaf()) {
        throw std::logic_error("Leaf node, cannot set child page ID");
    }
    if (index < 0 || index > header_.key_count) {
        throw std::out_of_range("Child index out of range: " + std::to_string(index));
    }
    
    size_t offset = get_value_offset(index);
    char* data = get_data_start();
    std::memcpy(data + offset, &page_id, sizeof(PageID));
    mark_dirty();
}

/**
 * @brief 在节点中查找键的位置
 * @param key 要查找的键值
 * @return 如果找到键，返回非负索引；否则返回负的插入位置提示
 *         具体为：-index-1 表示应该插入的位置
 */
int BPlusTreePage::find_key_index(const Value& key) const {
    // 使用线性查找（后续可优化为二分查找以提高性能）
    for (int i = 0; i < header_.key_count; ++i) {
        Value current_key = get_key_at(i);
        if (current_key == key) {
            return i; // 精确匹配，返回正数索引
        }
        if (current_key > key) {
            return -i - 1; // 返回插入位置提示
        }
    }
    return -header_.key_count - 1; // 插入到最后
}

/**
 * @brief 向叶子节点插入键值对
 * @param key 要插入的键
 * @param rid 要插入的记录ID
 * @return true 插入成功，false 插入失败（节点已满或不是叶子节点）
 */
bool BPlusTreePage::insert_leaf_pair(const Value& key, const RID& rid) {
    if (!is_leaf() || is_full()) {
        return false;
    }
    
    // 找到合适的插入位置
    int insert_pos = find_key_index(key);
    if (insert_pos < 0) {
        insert_pos = -insert_pos - 1;
    }
    
    // 为插入新元素腾出空间：向右移动插入位置后的所有元素
    for (int i = header_.key_count; i > insert_pos; --i) {
        // 移动键
        Value key_to_move = get_key_at(i - 1);
        set_key_at(i, key_to_move);
        
        // 移动RID
        RID rid_to_move = get_rid_at(i - 1);
        set_rid_at(i, rid_to_move);
    }
    
    // 在空出的位置插入新键值对
    set_key_at(insert_pos, key);
    set_rid_at(insert_pos, rid);
    
    header_.key_count++;
    mark_dirty();
    return true;
}

/**
 * @brief 向内部节点插入键和子指针
 * @param key 要插入的键（分隔键）
 * @param child_page_id 要插入的子页面ID
 * @return true 插入成功，false 插入失败（节点已满或是叶子节点）
 */
bool BPlusTreePage::insert_internal_pair(const Value& key, PageID child_page_id) {
    if (is_leaf() || is_full()) {
        return false;
    }
    
    // 找到合适的插入位置
    int insert_pos = find_key_index(key);
    if (insert_pos < 0) {
        insert_pos = -insert_pos - 1;
    }
    
    // 移动现有键和指针，为插入腾出空间
    // 注意：内部节点有n+1个指针，比键多一个
    for (int i = header_.key_count; i > insert_pos; --i) {
        // 移动键
        Value key_to_move = get_key_at(i - 1);
        set_key_at(i, key_to_move);
        
        // 移动指针（比键多一个）
        PageID ptr_to_move = get_child_page_id_at(i);
        set_child_page_id_at(i + 1, ptr_to_move);
    }
    
    // 插入新键和指针
    set_key_at(insert_pos, key);
    set_child_page_id_at(insert_pos + 1, child_page_id);
    
    header_.key_count++;
    mark_dirty();
    return true;
}

/**
 * @brief 检查节点是否已满
 * @return true 节点已满，false 节点还有空间
 */
bool BPlusTreePage::is_full() const {
    // 简易容量计算：基于固定大小的键和值
    const size_t KEY_SIZE = sizeof(int32_t); // 假设键是int32类型
    const size_t VALUE_SIZE = is_leaf() ? sizeof(RID) : sizeof(PageID);
    const size_t PAIR_SIZE = KEY_SIZE + VALUE_SIZE;
    
    const size_t MAX_CAPACITY = (PAGE_SIZE - sizeof(BPlusNodeHeader)) / PAIR_SIZE;
    return header_.key_count >= MAX_CAPACITY;
}

/**
 * @brief 获取节点的剩余空间大小（字节）
 * @return 剩余空间字节数
 */
uint16_t BPlusTreePage::get_free_space() const {
    const size_t KEY_SIZE = sizeof(int32_t);
    const size_t VALUE_SIZE = is_leaf() ? sizeof(RID) : sizeof(PageID);
    const size_t PAIR_SIZE = KEY_SIZE + VALUE_SIZE;
    
    const size_t TOTAL_SPACE = PAGE_SIZE - sizeof(BPlusNodeHeader);
    const size_t USED_SPACE = header_.key_count * PAIR_SIZE;
    
    return static_cast<uint16_t>(TOTAL_SPACE - USED_SPACE);
}

/**
 * @brief 获取节点数据区域的起始地址
 * @return 指向数据区域开始的指针
 */
char* BPlusTreePage::get_data_start() const {
    return const_cast<char*>(page_->getData()) + sizeof(BPlusNodeHeader);
}

/**
 * @brief 获取每个键值对的大小（字节）
 * @return 键值对的大小
 */
size_t BPlusTreePage::get_pair_size() const {
    const size_t KEY_SIZE = sizeof(int32_t);
    const size_t VALUE_SIZE = is_leaf() ? sizeof(RID) : sizeof(PageID);
    return KEY_SIZE + VALUE_SIZE;
}

/**
 * @brief 计算指定索引键的偏移量
 * @param index 键的索引
 * @return 键在数据区域中的字节偏移量
 */
size_t BPlusTreePage::get_key_offset(int index) const {
    return index * get_pair_size();
}

/**
 * @brief 计算指定索引值的偏移量
 * @param index 值的索引
 * @return 值在数据区域中的字节偏移量
 */
size_t BPlusTreePage::get_value_offset(int index) const {
    const size_t KEY_SIZE = sizeof(int32_t);
    return index * get_pair_size() + KEY_SIZE;
}

/**
 * @brief 序列化值到字节缓冲区
 * @param value 要序列化的值
 * @param buffer 目标缓冲区
 * @throw std::runtime_error 不支持的值的类型
 */
void BPlusTreePage::serialize_value(const Value& value, char* buffer) const {
    // 简化实现：只处理int32类型
    if (value.getType() == TypeId::INTEGER) {
        int32_t int_value = value.getAsInt();
        std::memcpy(buffer, &int_value, sizeof(int32_t));
    } else {
        throw std::runtime_error("Unsupported value type for serialization");
    }
}

/**
 * @brief 从字节缓冲区反序列化值
 * @param buffer 源缓冲区
 * @param type 值的类型
 * @return 反序列化后的Value对象
 * @throw std::runtime_error 不支持的值的类型
 */
Value BPlusTreePage::deserialize_value(const char* buffer, TypeId type) const {
    // 简化实现：只处理int32类型
    if (type == TypeId::INTEGER) {
        int32_t int_value;
        std::memcpy(&int_value, buffer, sizeof(int32_t));
        return Value(int_value);
    } else {
        throw std::runtime_error("Unsupported value type for deserialization");
    }
}

} // namespace engine
} // namespace minidb