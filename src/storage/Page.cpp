#include "storage/Page.h"
#include "common/Exception.h"
#include "common/Constants.h"
#include <cstring>
#include <sstream>

namespace minidb {
namespace storage {

// ====================== 构造函数 ======================
    Page::Page() : pin_count_(0) {
        header_.page_id = INVALID_PAGE_ID;
        header_.page_type = PageType::DATA_PAGE;
        // 关键修复：初始空闲空间偏移 = 数据区起始位置（槽位从0开始分配）
        header_.free_space_offset = 0;  // 数据区开头用于存储槽位，空闲空间从0开始向后分配
        // 数据区总大小 = 页大小 - 页头大小（确保槽位和记录有足够空间）
        header_.free_space = PAGE_SIZE - sizeof(PageHeader);
        header_.slot_count = 0;
        header_.is_dirty = false;
        header_.next_free_page = INVALID_PAGE_ID;

        memset(data_, 0, sizeof(data_));
    }

    Page::Page(PageID page_id) : Page() {
        header_.page_id = page_id;
    }

// ====================== 序列化/反序列化 ======================
void Page::serialize(char* dest) const {
    // 先拷贝页头（二进制拷贝，保证内存布局一致）
    memcpy(dest, &header_, sizeof(PageHeader));
    // 再拷贝数据区（包含槽位和记录数据）
    memcpy(dest + sizeof(PageHeader), data_, sizeof(data_));
}

void Page::deserialize(const char* src) {
    // 从二进制缓冲区读取页头
    memcpy(&header_, src, sizeof(PageHeader));
    // 从二进制缓冲区读取数据区
    memcpy(data_, src + sizeof(PageHeader), sizeof(data_));

        // // 添加额外的数据验证
        // if (header_.free_space_offset < 0 || header_.free_space_offset > sizeof(data_)) {
        //     throw PageException("Invalid page header data.");
        // }
}

// ====================== 记录操作辅助：空间检查 ======================
    bool Page::hasEnoughSpace(uint16_t required) const {
        // 修复：每个新记录仅需要1个新槽位（4字节），而非 (slot_count + 1) 个
        uint16_t total_needed = required + 4;  // required: 记录大小；4: 新槽位的空间（2字节偏移+2字节长度）
        return header_.free_space >= total_needed;
    }

// ====================== 记录操作：插入 ======================
    bool Page::insertRecord(const char* record_data, uint16_t record_size, RID* rid) {
        // 检查空间是否足够（修复后此处应能正确判断）
        if (!hasEnoughSpace(record_size)) {
            return false;
        }

        // 计算记录存储位置：从当前空闲空间末尾向前分配
        uint16_t record_offset = header_.free_space_offset + header_.free_space - record_size;
        // 拷贝记录数据到数据区
        memcpy(data_ + record_offset, record_data, record_size);

        // 新槽位号 = 当前槽位数量（插入前）
        uint16_t new_slot_num = header_.slot_count;
        // 写入槽位信息（偏移和大小）到数据区开头的槽位数组
        // 槽位存储格式：[偏移（2字节）][大小（2字节）]，按顺序存储在数据区开头
        uint16_t* slot_offset_ptr = reinterpret_cast<uint16_t*>(data_ + new_slot_num * 4);
        uint16_t* slot_size_ptr = slot_offset_ptr + 1;
        *slot_offset_ptr = record_offset;  // 记录在数据区的偏移
        *slot_size_ptr = record_size;      // 记录大小

        // 更新页头信息（关键：槽位数量+1）
        header_.slot_count++;
        header_.free_space -= (record_size + 4);  // 空闲空间减少：记录大小 + 槽位大小
        header_.is_dirty = true;

        // 设置返回的RID
        if (rid != nullptr) {
            rid->page_id = header_.page_id;
            rid->slot_num = new_slot_num;
        }

        return true;
    }

// ====================== 记录操作：查询 ======================
bool Page::getRecord(const RID& rid, char* buffer, uint16_t* size) const {
    // 校验RID有效性：页ID匹配 + 槽位号合法
    if (!rid.isValid() || rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
        return false;
    }

    uint16_t record_offset, record_size;
    // 获取槽位对应的“记录偏移”和“记录长度”
    if (!getSlotInfo(rid.slot_num, &record_offset, &record_size)) {
        return false;
    }

    // 将数据区的记录拷贝到输出缓冲区
    memcpy(buffer, data_ + record_offset, record_size);
    // 若需要返回记录长度，填充size
    if (size != nullptr) {
        *size = record_size;
    }

    return true;
}

// ====================== 记录操作：删除 ======================
bool Page::deleteRecord(const RID& rid) {
    // 校验RID有效性
    if (!rid.isValid() || rid.page_id != header_.page_id || rid.slot_num >= header_.slot_count) {
        return false;
    }

    uint16_t record_offset, record_size;
    // 获取槽位对应的“记录偏移”和“记录长度”
    if (!getSlotInfo(rid.slot_num, &record_offset, &record_size)) {
        return false;
    }

    // 简单删除：将槽位的“记录长度”设为0（标记为已删除，实际可后续压缩）
    setSlotSize(rid.slot_num, 0);
    header_.is_dirty = true; // 标记为脏页

    return true;
}

// ====================== 记录操作：更新 ======================
    bool Page::updateRecord(const RID& rid, const char* new_data, uint16_t new_size, RID* new_rid) {
    // 校验旧RID的有效性
    if (rid.slot_num >= header_.slot_count || rid.page_id != header_.page_id) {
        throw InvalidRIDException(rid);
    }

    // 标记旧记录为删除（仅置零长度，不删除槽位）
    setSlotSize(rid.slot_num, 0);

    // 插入新记录（会生成新槽位）
    bool inserted = insertRecord(new_data, new_size, new_rid);
    if (inserted) {
        header_.is_dirty = true;
    }
    return inserted;
}

// ====================== 槽位操作：辅助方法 ======================
    uint16_t Page::getSlotOffset(uint16_t slot_num) const {
    // 修复：slot_num 必须小于当前槽位总数（slot_count）
    if (slot_num >= header_.slot_count) {
        std::string msg = "Slot " + std::to_string(slot_num) + " out of range (page " +
                          std::to_string(header_.page_id) + "), total slots: " + std::to_string(header_.slot_count);
        throw InvalidSlotException(msg);
    }
    return *reinterpret_cast<const uint16_t*>(data_ + slot_num * 4);
}

    // 同理修复 setSlotOffset、getSlotSize、setSlotSize 方法的边界检查
    void Page::setSlotOffset(uint16_t slot_num, uint16_t offset) {
    if (slot_num >= header_.slot_count) {
        std::string msg = "Slot " + std::to_string(slot_num) + " out of range (page " +
                          std::to_string(header_.page_id) + "), total slots: " + std::to_string(header_.slot_count);
        throw InvalidSlotException(msg);
    }
    *reinterpret_cast<uint16_t*>(data_ + slot_num * 4) = offset;
}

    uint16_t Page::getSlotSize(uint16_t slot_num) const {
    if (slot_num >= header_.slot_count) {
        std::string msg = "Slot " + std::to_string(slot_num) + " out of range (page " +
                          std::to_string(header_.page_id) + "), total slots: " + std::to_string(header_.slot_count);
        throw InvalidSlotException(msg);
    }
    return *reinterpret_cast<const uint16_t*>(data_ + slot_num * 4 + 2);
}

    void Page::setSlotSize(uint16_t slot_num, uint16_t size) {
    if (slot_num >= header_.slot_count) {
        std::string msg = "Slot " + std::to_string(slot_num) + " out of range (page " +
                          std::to_string(header_.page_id) + "), total slots: " + std::to_string(header_.slot_count);
        throw InvalidSlotException(msg);
    }
    *reinterpret_cast<uint16_t*>(data_ + slot_num * 4 + 2) = size;
}

bool Page::getSlotInfo(uint16_t slot_num, uint16_t* offset, uint16_t* size) const {
    if (slot_num >= header_.slot_count) {
        return false;
    }
    *offset = getSlotOffset(slot_num);
    *size = getSlotSize(slot_num);
    return *size != 0; // 长度为0表示记录已被删除
}

    // ====================== 获取下一条记录 ======================
bool Page::getNextRecord(RID& rid) {
        if (!rid.isValid() || rid.slot_num >= header_.slot_count) {
            return false;
        }

        // 检查下一个槽位
        if (rid.slot_num + 1 < header_.slot_count) {
            rid.slot_num++; // 移动到下一个槽位
            return true; // 找到了下一条记录
        }

        return false; // 没有更多记录
    }
// ====================== 页面压缩（简化版，暂未实现完整逻辑） ======================
void Page::compactify() {
    throw NotImplementedException("Page compactify not implemented yet");
}

// ====================== 调试辅助：转为字符串 ======================
std::string Page::toString() const {
    std::stringstream ss;
    ss << "Page { ID: " << header_.page_id
       << ", Type: " << static_cast<int>(header_.page_type)
       << ", Slot Count: " << header_.slot_count
       << ", Free Space: " << header_.free_space
       << ", Dirty: " << (header_.is_dirty ? "true" : "false")
       << " }";
    return ss.str();
}

} // namespace storage
} // namespace minidb