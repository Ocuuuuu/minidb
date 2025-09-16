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
        uint16_t BPlusTreePage::calculate_key_size(TypeId type) const {
            switch (type) {
                case TypeId::BOOLEAN: return sizeof(bool);
                case TypeId::INTEGER: return sizeof(int32_t);
                case TypeId::VARCHAR: return 0;
                default: return sizeof(int32_t);
            }
        }

        BPlusTreePage::BPlusTreePage(storage::Page* page) : page_(page) {
            const char* data = page_->getData();
            std::memcpy(&header_, data, sizeof(BPlusNodeHeader));

            bool invalid_type = static_cast<int>(header_.key_type) < 0 ||
                                static_cast<int>(header_.key_type) > static_cast<int>(TypeId::VARCHAR);

            if (header_.key_count == 0 && invalid_type) {
                header_.key_type = TypeId::INTEGER;
                header_.key_size = calculate_key_size(TypeId::INTEGER);
                header_.is_leaf = false;
                header_.next_page_id = INVALID_PAGE_ID;
                save_header();
            }

            if (header_.key_size == 0) {
                header_.key_size = calculate_key_size(header_.key_type);
                save_header();
            }

            if (!is_leaf() && header_.key_count == 0) {
                set_child_page_id_at(0, INVALID_PAGE_ID);
            }
        }

        Value BPlusTreePage::get_key_at(int index) const {
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("Key index out of range");
            }
            return deserialize_key(get_data_start() + get_key_offset(index));
        }

        void BPlusTreePage::set_key_at(int index, const Value& key) {
            if (index < 0 || index >= header_.key_count) { // 改为 >=，不允许 index == key_count
                throw std::out_of_range("Key index out of range");
            }
            serialize_key(key, get_data_start() + get_key_offset(index));
            mark_dirty();
        }

        void BPlusTreePage::remove_key_at(int index) {
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("Key index out of range");
            }

            if (index < header_.key_count - 1) {
                shift_keys_left(index + 1);
            }

            header_.key_count--;
            save_header(); // ✅ 写回
            mark_dirty();
        }

        RID BPlusTreePage::get_rid_at(int index) const {
            if (!is_leaf()) throw std::logic_error("Not a leaf node");
            if (index < 0 || index >= header_.key_count) {
                throw std::out_of_range("RID index out of range");
            }
            return deserialize_rid(get_data_start() + get_value_offset(index));
        }

        void BPlusTreePage::set_rid_at(int index, const RID& rid) {
            if (!is_leaf()) throw std::logic_error("Not a leaf node");
            if (index < 0 || index >= header_.key_count) { // 同样改为 >=
                throw std::out_of_range("RID index out of range");
            }
            serialize_rid(rid, get_data_start() + get_value_offset(index));
            mark_dirty();
        }

        PageID BPlusTreePage::get_child_page_id_at(int index) const {
            if (is_leaf()) throw std::logic_error("Leaf node");
            if (index < 0 || index > header_.key_count) {
                throw std::out_of_range("Child index out of range");
            }
            return deserialize_page_id(get_data_start() + get_value_offset(index));
        }

        void BPlusTreePage::set_child_page_id_at(int index, PageID page_id) {
            if (is_leaf()) throw std::logic_error("Leaf node");
            if (index < 0 || index > header_.key_count + 1) {
                throw std::out_of_range("Child index out of range");
            }
            serialize_page_id(page_id, get_data_start() + get_value_offset(index));
            mark_dirty();
        }

        int BPlusTreePage::find_key_index(const Value& key) const {
            if (header_.key_count == 0) return -1;

            int left = 0, right = header_.key_count - 1;
            while (left <= right) {
                int mid = left + (right - left) / 2;
                Value mid_key = get_key_at(mid);

                if (mid_key == key) return mid;
                else if (mid_key < key) left = mid + 1;
                else right = mid - 1;
            }
            return -left - 1;
        }

        bool BPlusTreePage::insert_leaf_pair(const Value& key, const RID& rid) {
            if (!is_leaf() || is_full()) return false;

            int pos_hint = find_key_index(key);
            if (pos_hint >= 0) return false; // 已存在

            int insert_pos = -pos_hint - 1;
            if (insert_pos < 0 || insert_pos > header_.key_count) return false;

            header_.key_count++; // ✅ 先加
            if (insert_pos < header_.key_count - 1) { // 末尾插不用挪
                shift_keys_right(insert_pos);
                shift_values_right(insert_pos);
            }
            save_header();

            serialize_key(key, get_data_start() + get_key_offset(insert_pos));
            serialize_rid(rid, get_data_start() + get_value_offset(insert_pos));
            mark_dirty();
            return true;
        }


        bool BPlusTreePage::insert_internal_pair(const Value& key, PageID child_page_id) {
            if (is_leaf() || is_full()) return false;

            int pos_hint = find_key_index(key);
            if (pos_hint >= 0) return false;
            int insert_pos = -pos_hint - 1;

            header_.key_count++; // ✅ 先加
            if (insert_pos < header_.key_count - 1) {
                shift_keys_right(insert_pos);
                shift_values_right(insert_pos + 1);
            }
            save_header();

            serialize_key(key, get_data_start() + get_key_offset(insert_pos));
            serialize_page_id(child_page_id, get_data_start() + get_value_offset(insert_pos + 1));
            mark_dirty();
            return true;
        }


        bool BPlusTreePage::remove_leaf_pair(int index) {
            if (!is_leaf() || index < 0 || index >= header_.key_count) return false;
            remove_key_at(index);

            // 移动RID
            if (index < header_.key_count) {
                shift_values_left(index + 1);
            }
            mark_dirty();
            return true;
        }

        bool BPlusTreePage::is_full() const {
            return header_.key_count >= get_max_capacity();
        }
        uint16_t BPlusTreePage::get_free_space() const {
            size_t used_space = header_.key_count * get_pair_size();
            size_t total_space = PAGE_SIZE - sizeof(BPlusNodeHeader);
            return static_cast<uint16_t>(total_space - used_space);
        }

        bool BPlusTreePage::is_underflow() const {
            if (header_.key_count == 0) return true;

            int min_keys = get_max_capacity() / 2; // 典型的 B+树最小度数
            return header_.key_count < min_keys;
        }

        uint16_t BPlusTreePage::get_max_capacity() const {
            size_t pair_size = get_pair_size();
            if (pair_size == 0) return 0;

            size_t total_space = PAGE_SIZE - sizeof(BPlusNodeHeader);
            if (!is_leaf()) total_space -= get_value_size();

            return static_cast<uint16_t>(total_space / pair_size);
        }

        void BPlusTreePage::save_header() {
            std::memcpy(const_cast<char*>(page_->getData()), &header_, sizeof(BPlusNodeHeader));
            mark_dirty();
        }

        void BPlusTreePage::print_debug_info() const {
            std::cout << "Node Info: " << (is_leaf() ? "Leaf" : "Internal")
                      << ", KeyCount: " << header_.key_count
                      << ", NextPage: " << header_.next_page_id
                      << ", KeyType: " << static_cast<int>(header_.key_type)
                      << ", KeySize: " << header_.key_size
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
            // 对于固定长度类型，使用header中的key_size
            if (header_.key_size > 0) {
                return header_.key_size;
            }

            // 对于VARCHAR变长类型，需要特殊处理
            switch (header_.key_type) {
                case TypeId::VARCHAR:
                    return 256 + sizeof(uint16_t); // 字符串最大长度 + 长度字段
                default:
                    return sizeof(int32_t); // 默认
            }
        }

        size_t BPlusTreePage::get_value_size() const {
            return is_leaf() ? sizeof(RID) : sizeof(PageID);
        }

        size_t BPlusTreePage::get_pair_size() const {
            return get_key_size() + get_value_size();
        }

        size_t BPlusTreePage::get_key_offset(int index) const {
            if (header_.key_type != TypeId::VARCHAR) {
                // 固定长度类型：每个键值对的大小相同
                return index * get_pair_size();
            }

            // 对于VARCHAR类型，需要遍历计算偏移量
            size_t offset = 0;
            for (int i = 0; i < index; ++i) {
                const char* data = get_data_start() + offset;
                uint16_t len;
                std::memcpy(&len, data, sizeof(uint16_t));
                offset += sizeof(uint16_t) + len + get_value_size();
            }
            return offset;
        }

        size_t BPlusTreePage::get_value_offset(int index) const {
            if (header_.key_type != TypeId::VARCHAR) {
                // 固定长度类型：值在键之后
                return index * get_pair_size() + get_key_size();
            }

            // 对于VARCHAR类型，需要计算当前键的偏移量
            size_t key_offset = get_key_offset(index);
            const char* data = get_data_start() + key_offset;
            uint16_t len;
            std::memcpy(&len, data, sizeof(uint16_t));
            return key_offset + sizeof(uint16_t) + len;
        }

        void BPlusTreePage::serialize_key(const Value& key, char* buffer) const {
            if (key.getType() != header_.key_type) {
                throw std::runtime_error("Key type mismatch: expected " +
                                       std::to_string(static_cast<int>(header_.key_type)) +
                                       ", got " + std::to_string(static_cast<int>(key.getType())));
            }

            switch (header_.key_type) {
                case TypeId::BOOLEAN: {
                    bool bool_value = key.getAsBool();
                    std::memcpy(buffer, &bool_value, sizeof(bool));
                    break;
                }
                case TypeId::INTEGER: {
                    int32_t int_value = key.getAsInt();
                    std::memcpy(buffer, &int_value, sizeof(int32_t));
                    break;
                }
                case TypeId::VARCHAR: {
                    const std::string& str_value = key.getAsString();
                    uint16_t len = static_cast<uint16_t>(str_value.size());
                    // 先存储长度
                    std::memcpy(buffer, &len, sizeof(uint16_t));
                    // 再存储字符串内容
                    std::memcpy(buffer + sizeof(uint16_t), str_value.c_str(), len);
                    break;
                }
                case TypeId::INVALID:
                default:
                    throw std::runtime_error("Unsupported key type for serialization");
            }
        }

        Value BPlusTreePage::deserialize_key(const char* buffer) const {
            switch (header_.key_type) {
                case TypeId::BOOLEAN: {
                    bool bool_value;
                    std::memcpy(&bool_value, buffer, sizeof(bool));
                    return Value(bool_value);
                }
                case TypeId::INTEGER: {
                    int32_t int_value;
                    std::memcpy(&int_value, buffer, sizeof(int32_t));
                    return Value(int_value);
                }
                case TypeId::VARCHAR: {
                    uint16_t len;
                    std::memcpy(&len, buffer, sizeof(uint16_t));
                    std::string str_value(buffer + sizeof(uint16_t), len);
                    return Value(str_value);
                }
                case TypeId::INVALID:
                default:
                    throw std::runtime_error("Unsupported key type for deserialization");
            }
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
            if (start_index >= header_.key_count || count <= 0) return;
            char* data = get_data_start();
            size_t move_bytes = (header_.key_count - start_index) * get_key_size(); // ✅ 用实际 key_size
            std::memmove(data + get_key_offset(start_index + count),
                         data + get_key_offset(start_index),
                         move_bytes);
        }

        void BPlusTreePage::shift_keys_left(int start_index, int count) {
            if (start_index >= header_.key_count || count <= 0) return;

            char* data = get_data_start();
            size_t key_size = get_key_size();
            size_t pair_size = get_pair_size();

            int move_count = header_.key_count - start_index;
            char* src = data + get_key_offset(start_index);
            char* dest = src - count * pair_size;

            std::memmove(dest, src, move_count * key_size);
        }

        void BPlusTreePage::shift_values_right(int start_index, int count) {
            // 内部节点有key_count+1个指针，叶子节点有key_count个RID
            int total_values = is_leaf() ? header_.key_count : header_.key_count + 1;
            if (start_index >= total_values || count <= 0) return;

            char* data = get_data_start();
            size_t value_size = get_value_size();
            size_t pair_size = get_pair_size();

            int move_count = total_values - start_index;
            char* src = data + get_value_offset(start_index);
            char* dest = src + count * pair_size;

            std::memmove(dest, src, move_count * value_size);
        }

        void BPlusTreePage::shift_values_left(int start_index, int count) {
            int total_values = is_leaf() ? header_.key_count : header_.key_count + 1;
            if (start_index >= total_values || count <= 0) return;

            char* data = get_data_start();
            size_t value_size = get_value_size();
            size_t pair_size = get_pair_size();

            int move_count = total_values - start_index;
            char* src = data + get_value_offset(start_index);
            char* dest = src - count * pair_size;

            std::memmove(dest, src, move_count * value_size);
        }

        // 初始化页面数据
        void BPlusTreePage::initialize_page() {
            // 清空页面数据
            char* data = const_cast<char*>(page_->getData());
            std::memset(data, 0, PAGE_SIZE);

            // 初始化头信息
            header_.key_count = 0;
            header_.is_leaf = false;
            header_.next_page_id = 0;
            header_.key_type = TypeId::INTEGER;
            header_.key_size = calculate_key_size(TypeId::INTEGER);

            save_header();
            mark_dirty();
        }
    }//namespace engine
}//namespace minidb