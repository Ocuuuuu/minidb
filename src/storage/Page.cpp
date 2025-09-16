// storage/Page.cpp

#include "storage/Page.h"
#include "common/Exception.h"
#include "common/Constants.h"
#include <cstring>
#include <iostream>
#include <sstream>


namespace minidb {
namespace storage {

    Page::Page() : pin_count_(0) {
        header_.page_id = INVALID_PAGE_ID;
        header_.page_type = PageType::DATA_PAGE;
        header_.slot_count = 0;
        header_.is_dirty = false;
        header_.next_free_page = INVALID_PAGE_ID;

        // ✅ 修正初始化，去掉页头后的空间
        header_.free_space_offset = PAGE_SIZE - sizeof(PageHeader);
        header_.free_space = PAGE_SIZE - sizeof(PageHeader);

        memset(data_, 0, sizeof(data_));
    }


Page::Page(PageID page_id) : Page() {
    header_.page_id = page_id;
}

void Page::serialize(char* dest) const {
    std::memcpy(dest, &header_, sizeof(PageHeader));
    std::memcpy(dest + sizeof(PageHeader), data_, sizeof(data_));
}

void Page::deserialize(const char* src) {
    std::memcpy(&header_, src, sizeof(PageHeader));
    std::memcpy(data_, src + sizeof(PageHeader), sizeof(data_));
}

bool Page::hasEnoughSpace(uint16_t required) const {
    uint16_t total_needed = required + 4; // 记录数据 + 槽位元数据
    return header_.free_space >= total_needed;
}

    bool Page::insertRecord(const char* record_data, uint16_t record_size, RID* rid) {
        if (!hasEnoughSpace(record_size)) {
            return false;
        }

        // ✅ 从 data_ 顶部（free_space_offset）向下分配
        header_.free_space_offset -= record_size;
        uint16_t record_offset = header_.free_space_offset;

        // ✅ 越界保护
        if (record_offset + record_size > sizeof(data_)) {
            throw BufferPoolException("insertRecord 越界");
        }

        memcpy(data_ + record_offset, record_data, record_size);

        uint16_t new_slot_num = header_.slot_count;
        uint16_t* slot_offset_ptr = reinterpret_cast<uint16_t*>(data_ + new_slot_num * 4);
        uint16_t* slot_size_ptr   = slot_offset_ptr + 1;
        *slot_offset_ptr = record_offset;
        *slot_size_ptr   = record_size;

        header_.slot_count++;
        header_.free_space -= (record_size + 4);
        header_.is_dirty = true;

        if (rid) {
            rid->page_id = header_.page_id;
            rid->slot_num = new_slot_num;
        }
        std::cout << "[DEBUG insert] page_id=" << header_.page_id
          << " slot_num=" << new_slot_num
          << " offset=" << *slot_offset_ptr
          << " size=" << *slot_size_ptr
          << " slot_count=" << header_.slot_count
          << std::endl;

        return true;
    }


    bool Page::getRecord(const RID& rid, char* buffer, uint16_t* size) const {
        std::cout << "[DEBUG getRecord] call: rid.page_id=" << rid.page_id
                  << " rid.slot_num=" << rid.slot_num
                  << " header.page_id=" << header_.page_id
                  << " slot_count=" << header_.slot_count
                  << std::endl;

        if (!rid.isValid() || rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
            std::cout << "[DEBUG getRecord] return false" << std::endl;
            return false;
        }
        uint16_t record_offset, record_size;
        if (!getSlotInfo(rid.slot_num, &record_offset, &record_size)) {
            std::cout << "[DEBUG getRecord] invalid slot" << std::endl;
            return false;
        }
        std::memcpy(buffer, data_ + record_offset, record_size);
        if (size) *size = record_size;

        std::cout << "[DEBUG getRecord] return true, size=" << record_size << std::endl;
        return true;
    }

//
// bool Page::deleteRecord(const RID& rid) {
//     if (!rid.isValid() || rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
//         return false;
//     }
//     setSlotSize(rid.slot_num, 0);
//     header_.is_dirty = true;
//     return true;
// }

    bool Page::deleteRecord(const RID& rid) {
        uint16_t slot_num = rid.slot_num;
        if (slot_num >= header_.slot_count) {
            return false; // 非法slot号
        }

        uint16_t record_offset = getSlotOffset(slot_num);
        uint16_t record_size   = getSlotSize(slot_num);

        if (record_size == 0) {
            return false; // 已经空槽
        }

        // 可选: 清掉数据
        std::memset(data_ + record_offset, 0, record_size);

        // 核心：置 0
        setSlotSize(slot_num, 0);
        setSlotOffset(slot_num, 0);
        header_.free_space += record_size;
        header_.is_dirty = true;

        return true;
    }


bool Page::updateRecord(const RID& rid, const char* new_data, uint16_t new_size, RID* new_rid) {
    if (rid.slot_num >= header_.slot_count || rid.page_id != header_.page_id) {
        throw InvalidRIDException(rid);
    }
    setSlotSize(rid.slot_num, 0);
    bool inserted = insertRecord(new_data, new_size, new_rid);
    if (inserted) header_.is_dirty = true;
    return inserted;
}

// ====================== 槽位操作 ======================
uint16_t Page::getSlotOffset(uint16_t slot_num) const {
    if (slot_num >= header_.slot_count) {
        throw InvalidSlotException("Slot " + std::to_string(slot_num) +
                                   " out of range. Total slots: " + std::to_string(header_.slot_count) +
                                   ", Page ID: " + std::to_string(header_.page_id));
    }
    return *reinterpret_cast<const uint16_t*>(data_ + slot_num * 4);
}

void Page::setSlotOffset(uint16_t slot_num, uint16_t offset) {
    if (slot_num >= header_.slot_count) {
        throw InvalidSlotException("Slot " + std::to_string(slot_num) +
                                   " out of range. Total slots: " + std::to_string(header_.slot_count) +
                                   ", Page ID: " + std::to_string(header_.page_id));
    }
    *reinterpret_cast<uint16_t*>(data_ + slot_num * 4) = offset;
}

uint16_t Page::getSlotSize(uint16_t slot_num) const {
    if (slot_num >= header_.slot_count) {
        throw InvalidSlotException("Slot " + std::to_string(slot_num) +
                                   " out of range. Total slots: " + std::to_string(header_.slot_count) +
                                   ", Page ID: " + std::to_string(header_.page_id));
    }
    return *reinterpret_cast<const uint16_t*>(data_ + slot_num * 4 + 2);
}

void Page::setSlotSize(uint16_t slot_num, uint16_t size) {
    if (slot_num >= header_.slot_count) {
        throw InvalidSlotException("Slot " + std::to_string(slot_num) +
                                   " out of range. Total slots: " + std::to_string(header_.slot_count) +
                                   ", Page ID: " + std::to_string(header_.page_id));
    }
    *reinterpret_cast<uint16_t*>(data_ + slot_num * 4 + 2) = size;
}

bool Page::getSlotInfo(uint16_t slot_num, uint16_t* offset, uint16_t* size) const {
    if (slot_num >= header_.slot_count) {
        return false;
    }
    *offset = getSlotOffset(slot_num);
    *size = getSlotSize(slot_num);
    return *size != 0;  // 大小为0表示已删除
}

// ====================== 获取下一条记录 ======================
bool Page::getNextRecord(RID& rid) const {
        std::cout << "[DEBUG scan] page_id=" << header_.page_id
                  << " slot_count=" << header_.slot_count << std::endl;

        for (int i = 0; i < header_.slot_count; i++) {
            std::cout << " slot" << i
                      << " offset=" << getSlotOffset(i)
                      << " size="   << getSlotSize(i)
                      << std::endl;
        }

    if (!rid.isValid() || rid.slot_num >= header_.slot_count) {
        return false;
    }

    if (rid.slot_num + 1 < header_.slot_count) {
        rid.slot_num++;
        return true;
    }

    return false;
}

    // bool Page::getNextRecord(RID& rid) const {
    //     // 没有任何槽，直接返回 false
    //     if (header_.slot_count == 0) {
    //         return false;
    //     }
    //
    //     // 计算起始搜索位置
    //     uint16_t start = (rid.slot_num == static_cast<uint16_t>(-1))
    //                      ? 0
    //                      : rid.slot_num + 1;
    //
    //     for (uint16_t next = start; next < header_.slot_count; next++) {
    //         if (getSlotSize(next) > 0) { // 跳过已删除的记录
    //             rid.slot_num = next;
    //             return true;
    //         }
    //     }
    //     return false; // 没有更多记录
    // }
    //
    //

// ====================== 页面压缩（待实现） ======================
void Page::compactify() {
    throw NotImplementedException("Page compactify not implemented yet");
}

// ====================== 调试输出 ======================
std::string Page::toString() const {
    std::stringstream ss;
    ss << "Page { ID: " << header_.page_id
       << ", Type: " << static_cast<int>(header_.page_type)
       << ", Slot Count: " << header_.slot_count
       << ", Free Space: " << header_.free_space
       << ", Free Offset: " << header_.free_space_offset
       << ", Dirty: " << (header_.is_dirty ? "true" : "false")
       << " }";
    return ss.str();
}

} // namespace storage
} // namespace minidb