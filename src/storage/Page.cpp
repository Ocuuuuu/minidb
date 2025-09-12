//
// Created by tang_ on 2025/9/9.
//

#include "../../include/storage/Page.h"
#include "storage/Page.h"
#include <cstring>
#include <sstream>

namespace minidb {
namespace storage {

Page::Page() {
    std::memset(&header_, 0, sizeof(PageHeader));
    std::memset(data_, 0, sizeof(data_));
    header_.free_space = sizeof(data_);
}

Page::Page(PageID page_id) {
    std::memset(&header_, 0, sizeof(PageHeader));
    std::memset(data_, 0, sizeof(data_));
    header_.page_id = page_id;
    header_.free_space = sizeof(data_);
}

bool Page::insertRecord(const char* record_data, uint16_t record_size, RID* rid) {
    // 计算所需空间：记录数据 + 槽位信息(4字节)
    uint16_t required_space = record_size + 4;

    if (!hasEnoughSpace(required_space)) {
        // 尝试压缩页面
        compactify();

        // 再次检查空间
        if (!hasEnoughSpace(required_space)) {
            return false; // 空间不足
        }
    }

    // 计算新记录的偏移量
    uint16_t record_offset = sizeof(data_) - header_.free_space;

    // 复制记录数据
    std::memcpy(data_ + record_offset, record_data, record_size);

    // 更新槽位信息
    uint16_t slot_offset = sizeof(data_) - 4 * (header_.slot_count + 1);
    setSlotOffset(header_.slot_count, record_offset);
    setSlotSize(header_.slot_count, record_size);

    // 更新页面头
    header_.slot_count++;
    header_.free_space -= (record_size + 4);
    header_.free_space_offset = record_offset + record_size;

    // 设置返回的RID
    if (rid) {
        rid->page_id = header_.page_id;
        rid->slot_num = header_.slot_count - 1;
    }

    header_.is_dirty = true;
    return true;
}

bool Page::getRecord(const RID& rid, char* buffer, uint16_t* size) const {
    if (rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
        return false;
    }

    uint16_t offset, record_size;
    if (!getSlotInfo(rid.slot_num, &offset, &record_size)) {
        return false;
    }

    if (size) {
        *size = record_size;
    }

    std::memcpy(buffer, data_ + offset, record_size);
    return true;
}

bool Page::deleteRecord(const RID& rid) {
    if (rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
        return false;
    }

    // 标记槽位大小为0表示已删除
    setSlotSize(rid.slot_num, 0);

    header_.is_dirty = true;
    return true;
}

bool Page::updateRecord(const RID& rid, const char* new_data, uint16_t new_size) {
    if (rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
        return false;
    }

    uint16_t old_offset, old_size;
    if (!getSlotInfo(rid.slot_num, &old_offset, &old_size)) {
        return false;
    }

    // 如果新数据大小与旧数据相同，可以直接覆盖
    if (new_size == old_size) {
        std::memcpy(data_ + old_offset, new_data, new_size);
        header_.is_dirty = true;
        return true;
    }

    // 如果大小不同，需要先删除旧记录，再插入新记录
    if (!deleteRecord(rid)) {
        return false;
    }

    RID new_rid;
    if (!insertRecord(new_data, new_size, &new_rid)) {
        return false;
    }

    // 更新RID（如果需要）
    // 注意：在实际系统中，可能需要更新所有指向旧RID的引用

    return true;
}

std::string Page::toString() const {
    std::stringstream ss;
    ss << "Page " << header_.page_id
       << " [Slots: " << header_.slot_count
       << ", Free: " << header_.free_space
       << ", Dirty: " << (header_.is_dirty ? "Yes" : "No") << "]";
    return ss.str();
}

void Page::serialize(char* dest) const {
    // 复制页头
    std::memcpy(dest, &header_, sizeof(PageHeader));

    // 复制数据
    std::memcpy(dest + sizeof(PageHeader), data_, sizeof(data_));
}

void Page::deserialize(const char* src) {
    // 读取页头
    std::memcpy(&header_, src, sizeof(PageHeader));

    // 读取数据
    std::memcpy(data_, src + sizeof(PageHeader), sizeof(data_));
}

uint16_t Page::getSlotOffset(uint16_t slot_num) const {
    if (slot_num >= header_.slot_count) {
        return 0;
    }

    uint16_t slot_pos = sizeof(data_) - 4 * (slot_num + 1);
    return *reinterpret_cast<const uint16_t*>(data_ + slot_pos);
}

void Page::setSlotOffset(uint16_t slot_num, uint16_t offset) {
    if (slot_num >= header_.slot_count) {
        return;
    }

    uint16_t slot_pos = sizeof(data_) - 4 * (slot_num + 1);
    *reinterpret_cast<uint16_t*>(data_ + slot_pos) = offset;
}

uint16_t Page::getSlotSize(uint16_t slot_num) const {
    if (slot_num >= header_.slot_count) {
        return 0;
    }

    uint16_t slot_pos = sizeof(data_) - 4 * (slot_num + 1) + 2;
    return *reinterpret_cast<const uint16_t*>(data_ + slot_pos);
}

void Page::setSlotSize(uint16_t slot_num, uint16_t size) {
    if (slot_num >= header_.slot_count) {
        return;
    }

    uint16_t slot_pos = sizeof(data_) - 4 * (slot_num + 1) + 2;
    *reinterpret_cast<uint16_t*>(data_ + slot_pos) = size;
}

bool Page::getSlotInfo(uint16_t slot_num, uint16_t* offset, uint16_t* size) const {
    if (slot_num >= header_.slot_count) {
        return false;
    }

    uint16_t slot_pos = sizeof(data_) - 4 * (slot_num + 1);
    if (offset) {
        *offset = *reinterpret_cast<const uint16_t*>(data_ + slot_pos);
    }
    if (size) {
        *size = *reinterpret_cast<const uint16_t*>(data_ + slot_pos + 2);
    }
    return true;
}

void Page::compactify() {
    // 创建临时缓冲区
    char temp_data[sizeof(data_)];
    std::memset(temp_data, 0, sizeof(temp_data));

    uint16_t current_offset = 0;

    // 重新组织有效记录
    for (uint16_t i = 0; i < header_.slot_count; i++) {
        uint16_t offset, size;
        if (getSlotInfo(i, &offset, &size) && size > 0) {
            // 复制有效记录到新位置
            std::memcpy(temp_data + current_offset, data_ + offset, size);

            // 更新槽位偏移
            setSlotOffset(i, current_offset);

            // 更新当前偏移
            current_offset += size;
        }
    }

    // 更新空闲空间信息
    header_.free_space = sizeof(data_) - current_offset - 4 * header_.slot_count;
    header_.free_space_offset = current_offset;

    // 复制回数据区域
    std::memcpy(data_, temp_data, current_offset);

    // 清空剩余空间
    std::memset(data_ + current_offset, 0, sizeof(data_) - current_offset);

    header_.is_dirty = true;
}

} // namespace storage
} // namespace minidb