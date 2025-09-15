#include "engine/bPlusTree/bplus_tree_page.h"
#include "storage/Page.h"
#include "common/Value.h"
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace minidb
{
    namespace engine
    {
        BPlusTreePage::BPlusTreePage(storage::Page* page) : page_(page) {
            const char* data = page_->getData();
            std::memcpy(&header_, data, sizeof(BPlusNodeHeader));
        }

        Value BPlusTreePage::get_key_at(int index) const {
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("Key index out of range: " + std::to_string(index));
            }

            size_t offset = get_key_offset(index);
            const char* data = get_data_start();
            return deserialize_key(data + offset);
        }

        void BPlusTreePage::set_key_at(int index, const Value& key) {
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("Key index out of range: " + std::to_string(index));
            }

            size_t offset = get_key_offset(index);
            char* data = get_data_start();
            serialize_key(key, data + offset);
            mark_dirty();
        }

        void BPlusTreePage::remove_key_at(int index) {
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("Key index out of range: " + std::to_string(index));
            }

            // 向左移动后面的键
            shift_keys_left(index + 1);
            header_.key_count--;
            mark_dirty();
        }

        RID BPlusTreePage::get_rid_at(int index) const {
            if (!is_leaf()) {
                throw std::logic_error("Not a leaf node, cannot get RID");
            }
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("RID index out of range: " + std::to_string(index));
            }

            size_t offset = get_value_offset(index);
            const char* data = get_data_start();
            return deserialize_rid(data + offset);
        }

        void BPlusTreePage::set_rid_at(int index, const RID& rid) {
            if (!is_leaf()) {
                throw std::logic_error("Not a leaf node, cannot set RID");
            }
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("RID index out of range: " + std::to_string(index));
            }

            size_t offset = get_value_offset(index);
            char* data = get_data_start();
            serialize_rid(rid, data + offset);
            mark_dirty();
        }

        void BPlusTreePage::remove_rid_at(int index) {
            if (!is_leaf()) {
                throw std::logic_error("Not a leaf node, cannot remove RID");
            }
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("RID index out of range: " + std::to_string(index));
            }

            // 向左移动后面的RID
            shift_values_left(index + 1);
            mark_dirty();
        }

        PageID BPlusTreePage::get_child_page_id_at(int index) const {
            if (is_leaf()) {
                throw std::logic_error("Leaf node, cannot get child page ID");
            }
            if (index < 0 || index > header_.key_count) {
                throw std::out_of_range("Child index out of range: " + std::to_string(index));
            }

            size_t offset = get_value_offset(index);
            const char* data = get_data_start();
            return deserialize_page_id(data + offset);
        }

        void BPlusTreePage::set_child_page_id_at(int index, PageID page_id) {
            if (is_leaf()) {
                throw std::logic_error("Leaf node, cannot set child page ID");
            }
            if (index < 0 || index > header_.key_count) {
                throw std::out_of_range("Child index out of range: " + std::to_string(index));
            }

            size_t offset = get_value_offset(index);
            char* data = get_data_start();
            serialize_page_id(page_id, data + offset);
            mark_dirty();
        }

        void BPlusTreePage::remove_child_page_id_at(int index) {
            if (is_leaf()) {
                throw std::logic_error("Leaf node, cannot remove child page ID");
            }
            if (index < 0 || index > header_.key_count) {
                throw std::out_of_range("Child index out of range: " + std::to_string(index));
            }

            // 向左移动后面的指针
            shift_values_left(index + 1);
            mark_dirty();
        }

        int BPlusTreePage::find_key_index(const Value& key) const {
            // 使用二分查找提高性能
            int left = 0;
            int right = header_.key_count - 1;

            while (left <= right) {
                int mid = left + (right - left) / 2;
                Value mid_key = get_key_at(mid);

                if (mid_key == key) {
                    return mid; // 精确匹配
                } else if (mid_key < key) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }

            return -left - 1; // 返回插入位置提示
        }

        bool BPlusTreePage::insert_leaf_pair(const Value& key, const RID& rid) {
            if (!is_leaf() || is_full()) {
                return false;
            }

            int insert_pos = find_key_index(key);
            if (insert_pos >= 0) {
                // 键已存在，不允许重复
                return false;
            }
            insert_pos = -insert_pos - 1;

            // 为插入腾出空间
            if (insert_pos < header_.key_count) {
                shift_keys_right(insert_pos);
                shift_values_right(insert_pos);
            }

            // 插入新键值对
            set_key_at(insert_pos, key);
            set_rid_at(insert_pos, rid);

            header_.key_count++;
            mark_dirty();
            return true;
        }

        bool BPlusTreePage::insert_internal_pair(const Value& key, PageID child_page_id) {
            if (is_leaf() || is_full()) {
                return false;
            }

            int insert_pos = find_key_index(key);
            if (insert_pos >= 0) {
                // 键已存在
                return false;
            }
            insert_pos = -insert_pos - 1;

            // 移动键和指针为插入腾出空间
            // 注意：内部节点有n+1个指针，比键多一个

            // 先移动指针（比键多一个）
            if (insert_pos <= header_.key_count) {
                shift_values_right(insert_pos + 1);
            }

            // 再移动键
            if (insert_pos < header_.key_count) {
                shift_keys_right(insert_pos);
            }

            // 插入新键和指针
            set_key_at(insert_pos, key);
            set_child_page_id_at(insert_pos + 1, child_page_id);

            header_.key_count++;
            mark_dirty();
            return true;
        }

        bool BPlusTreePage::remove_leaf_pair(int index) {
            if (!is_leaf() || index < 0 || index >= header_.key_count) {
                return false;
            }

            remove_key_at(index);
            remove_rid_at(index);
            return true;
        }

        bool BPlusTreePage::remove_internal_pair(int index) {
            if (is_leaf() || index < 0 || index >= header_.key_count) {
                return false;
            }

            remove_key_at(index);
            remove_child_page_id_at(index + 1);
            return true;
        }

        bool BPlusTreePage::is_full() const {
            return header_.key_count >= get_max_capacity();
        }

        bool BPlusTreePage::is_underflow() const {
            // 简单实现：节点少于一半容量时认为下溢
            return header_.key_count < get_max_capacity() / 2;
        }

        uint16_t BPlusTreePage::get_free_space() const {
            size_t used_space = header_.key_count * get_pair_size();
            size_t total_space = PAGE_SIZE - sizeof(BPlusNodeHeader);
            return static_cast<uint16_t>(total_space - used_space);
        }

        uint16_t BPlusTreePage::get_max_capacity() const {
            size_t pair_size = get_pair_size();
            if (pair_size == 0) return 0;

            size_t total_space = PAGE_SIZE - sizeof(BPlusNodeHeader);
            // 内部节点需要额外空间存储第一个指针
            if (!is_leaf()) {
                total_space -= get_value_size();
            }

            return static_cast<uint16_t>(total_space / pair_size);
        }

        void BPlusTreePage::save_header() {
            char* data = const_cast<char*>(page_->getData());
            std::memcpy(data, &header_, sizeof(BPlusNodeHeader));
            mark_dirty();
        }

        void BPlusTreePage::print_debug_info() const {
            std::cout << "Node Info: " << (is_leaf() ? "Leaf" : "Internal")
                      << ", KeyCount: " << header_.key_count
                      << ", NextPage: " << header_.next_page_id
                      << ", MaxCapacity: " << get_max_capacity()
                      << std::endl;

            for (int i = 0; i < header_.key_count; ++i) {
                Value key = get_key_at(i);
                std::cout << "  Key[" << i << "]: " << key.toString();
                if (is_leaf()) {
                    RID rid = get_rid_at(i);
                    std::cout << " -> RID: " << rid.toString();
                } else {
                    PageID child_id = get_child_page_id_at(i);
                    std::cout << " -> Child: " << child_id;
                }
                std::cout << std::endl;
            }

            if (!is_leaf() && header_.key_count >= 0) {
                PageID last_child = get_child_page_id_at(header_.key_count);
                std::cout << "  Last Child: " << last_child << std::endl;
            }
        }

        char* BPlusTreePage::get_data_start() const {
            return const_cast<char*>(page_->getData()) + sizeof(BPlusNodeHeader);
        }

        size_t BPlusTreePage::get_key_size() const {
            // 假设所有键都是INTEGER类型，实际应该根据key_type确定
            return sizeof(int32_t);
        }

        size_t BPlusTreePage::get_value_size() const {
            return is_leaf() ? sizeof(RID) : sizeof(PageID);
        }

        size_t BPlusTreePage::get_pair_size() const {
            return get_key_size() + get_value_size();
        }

        size_t BPlusTreePage::get_key_offset(int index) const {
            return index * get_pair_size();
        }

        size_t BPlusTreePage::get_value_offset(int index) const {
            return index * get_pair_size() + get_key_size();
        }

        void BPlusTreePage::serialize_key(const Value& key, char* buffer) const {
            if (key.getType() == TypeId::INTEGER) {
                int32_t int_value = key.getAsInt();
                std::memcpy(buffer, &int_value, sizeof(int32_t));
            } else {
                throw std::runtime_error("Unsupported key type for serialization");
            }
        }

        Value BPlusTreePage::deserialize_key(const char* buffer) const {
            // 假设键都是INTEGER类型
            int32_t int_value;
            std::memcpy(&int_value, buffer, sizeof(int32_t));
            return Value(int_value);
        }

        void BPlusTreePage::serialize_rid(const RID& rid, char* buffer) const {
            std::memcpy(buffer, &rid, sizeof(RID));
        }

        RID BPlusTreePage::deserialize_rid(const char* buffer) const {
            RID rid;
            std::memcpy(&rid, buffer, sizeof(RID));
            return rid;
        }

        void BPlusTreePage::serialize_page_id(PageID page_id, char* buffer) const {
            std::memcpy(buffer, &page_id, sizeof(PageID));
        }

        PageID BPlusTreePage::deserialize_page_id(const char* buffer) const {
            PageID page_id;
            std::memcpy(&page_id, buffer, sizeof(PageID));
            return page_id;
        }

        void BPlusTreePage::shift_keys_right(int start_index, int count) {
            if (start_index >= header_.key_count) return;

            char* data = get_data_start();
            size_t key_size = get_key_size();
            size_t pair_size = get_pair_size();

            int move_count = header_.key_count - start_index;
            char* src = data + get_key_offset(start_index);
            char* dest = src + count * pair_size;

            std::memmove(dest, src, move_count * key_size);
        }

        void BPlusTreePage::shift_keys_left(int start_index, int count) {
            if (start_index >= header_.key_count) return;

            char* data = get_data_start();
            size_t key_size = get_key_size();
            size_t pair_size = get_pair_size();

            int move_count = header_.key_count - start_index;
            char* src = data + get_key_offset(start_index);
            char* dest = src - count * pair_size;

            std::memmove(dest, src, move_count * key_size);
        }

        void BPlusTreePage::shift_values_right(int start_index, int count) {
            if (start_index > header_.key_count) return;

            char* data = get_data_start();
            size_t value_size = get_value_size();
            size_t pair_size = get_pair_size();

            int move_count = (is_leaf() ? header_.key_count : header_.key_count + 1) - start_index;
            char* src = data + get_value_offset(start_index);
            char* dest = src + count * pair_size;

            std::memmove(dest, src, move_count * value_size);
        }

        void BPlusTreePage::shift_values_left(int start_index, int count) {
            if (start_index > header_.key_count) return;

            char* data = get_data_start();
            size_t value_size = get_value_size();
            size_t pair_size = get_pair_size();

            int move_count = (is_leaf() ? header_.key_count : header_.key_count + 1) - start_index;
            char* src = data + get_value_offset(start_index);
            char* dest = src - count * pair_size;

            std::memmove(dest, src, move_count * value_size);
        }
    }//namespace egine
}//namespace minidb