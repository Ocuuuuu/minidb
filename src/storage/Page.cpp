// storage/Page.cpp

#include "storage/Page.h"
#include "common/Exception.h"
#include "common/Constants.h"
#include <cstring>
#include <sstream>
#include <iostream>
namespace minidb {
namespace storage {

// ====================== 构造函数 ======================
    Page::Page() : pin_count_(0) {
        header_.page_id = INVALID_PAGE_ID;
        header_.page_type = PageType::DATA_PAGE;
        header_.slot_count = 0;
        header_.is_dirty = false;
        header_.next_free_page = INVALID_PAGE_ID;

        // ✅ 正确初始化：free_space_offset 应该是数据区的起始偏移
        header_.free_space_offset = 0;  // 从数据区开头开始使用
        header_.free_space = sizeof(data_);  // 总可用空间 = 数据区大小

        memset(data_, 0, sizeof(data_));
    }

    Page::Page(PageID page_id) : Page() {
        header_.page_id = page_id;
    }

    // ====================== 序列化/反序列化 ======================
    void Page::serialize(char* dest) const {
        // 只序列化应该持久化的部分：header_ + data_
        memcpy(dest, &header_, sizeof(PageHeader));
        memcpy(dest + sizeof(PageHeader), data_, sizeof(data_));
    }

    void Page::deserialize(const char* src) {
        // 只反序列化持久化的部分，不要覆盖pin_count_
        memcpy(&header_, src, sizeof(PageHeader));
        memcpy(data_, src + sizeof(PageHeader), sizeof(data_));
        // pin_count_ 保持不变！
    }

// ====================== 空间检查 ======================
bool Page::hasEnoughSpace(uint16_t required) const {
    uint16_t total_needed = required + 4;  // 记录大小 + 槽位元数据（4字节）
    return header_.free_space >= total_needed;
}

    // ====================== 插入记录 ======================
    bool Page::insertRecord(const char* record_data, uint16_t record_size, RID* rid) {
    if (!hasEnoughSpace(record_size)) {
        return false;
    }

    // ✅ 从 data_ 的末尾开始向下分配
    header_.free_space_offset -= record_size;
    uint16_t record_offset = header_.free_space_offset;

    // ✅ 现在 record_offset <= PAGE_SIZE - sizeof(PageHeader)，不会越界
    memcpy(data_ + record_offset, record_data, record_size);

    // 插入槽位（在 data_ 起始处）
    uint16_t new_slot_num = header_.slot_count;
    uint16_t* slot_offset_ptr = reinterpret_cast<uint16_t*>(data_ + new_slot_num * 4);
    uint16_t* slot_size_ptr = slot_offset_ptr + 1;
    *slot_offset_ptr = record_offset;
    *slot_size_ptr = record_size;

    header_.slot_count++;
    header_.free_space -= (record_size + 4);
    header_.is_dirty = true;

    if (rid) {
        rid->page_id = header_.page_id;
        rid->slot_num = new_slot_num;
    }

    return true;
}

    // ====================== 获取记录 ======================
    bool Page::getRecord(const RID& rid, char* buffer, uint16_t* size) const {
    if (!rid.isValid() || rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
        return false;
    }

    uint16_t record_offset, record_size;
    if (!getSlotInfo(rid.slot_num, &record_offset, &record_size)) {
        return false;
    }

    memcpy(buffer, data_ + record_offset, record_size);
    if (size != nullptr) {
        *size = record_size;
    }

    return true;
}

// ====================== 删除记录 ======================
bool Page::deleteRecord(const RID& rid) {
    if (!rid.isValid() || rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
        return false;
    }

    setSlotSize(rid.slot_num, 0);
    header_.is_dirty = true;
    return true;
}

// ====================== 更新记录 ======================
bool Page::updateRecord(const RID& rid, const char* new_data, uint16_t new_size, RID* new_rid) {
    if (rid.slot_num >= header_.slot_count || rid.page_id != header_.page_id) {
        throw InvalidRIDException(rid);
    }

    setSlotSize(rid.slot_num, 0);  // 标记旧记录为删除

    bool inserted = insertRecord(new_data, new_size, new_rid);
    if (inserted) {
        header_.is_dirty = true;
    }
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
    if (!rid.isValid() || rid.slot_num >= header_.slot_count) {
        return false;
    }

    if (rid.slot_num + 1 < header_.slot_count) {
        rid.slot_num++;
        return true;
    }

    return false;
}

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