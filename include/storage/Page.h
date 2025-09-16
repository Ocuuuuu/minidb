#ifndef MINIDB_PAGE_H
#define MINIDB_PAGE_H

#include "common/Constants.h"   // 包含 PAGE_SIZE、INVALID_PAGE_ID 等常量
#include "common/Types.h"       // 包含 PageID、RID 等类型定义
#include <cstdint>              // 包含 uint16_t 等整数类型
#include <cstring>
#include <string>               // 用于 toString 函数返回值

namespace minidb {
namespace storage {

// 页面类型枚举（与 cpp 中 page_type 成员匹配）
enum class PageType {
    DATA_PAGE,    // 数据页（默认类型）
    INDEX_PAGE,   // 索引页
    FREE_PAGE,    // 空闲页
    META_PAGE     // 元数据页
};

// 页面头部结构体（与 cpp 中 header_ 成员的初始化逻辑匹配）
#pragma pack(push, 1)  // 按1字节对齐，避免内存浪费（与磁盘存储一致）
struct PageHeader {
    PageID page_id = INVALID_PAGE_ID;          // 页面唯一标识（默认无效值）
    PageType page_type = PageType::DATA_PAGE;  // 页面类型（默认数据页）
    uint16_t free_space_offset = 0;            // 空闲空间起始偏移（cpp 中初始化为 sizeof(data_)）
    uint16_t slot_count = 0;                   // 槽位数量（记录数量，初始为0）
    uint16_t free_space = 0;                   // 剩余空闲空间（字节，cpp 中初始化为 sizeof(data_)）
    bool is_dirty = false;                     // 是否为脏页（内存与磁盘不一致）
    PageID next_free_page = INVALID_PAGE_ID;   // 空闲链表中下一个空闲页ID（默认无效）
};
#pragma pack(pop)

class Page {
public:
    // 1. 构造函数（与 cpp 实现完全匹配）
    Page();                                  // 默认构造：pin_count=0，页头/数据区清零
    explicit Page(PageID page_id);           // 带页ID的构造：指定 page_id，其他同默认构造


    // ✅ 新增的方法
    uint16_t getSlotCount() const {
        return header_.slot_count;
    }
    uint16_t getSlotSize(uint16_t slot_num) const;    // 获取指定槽位的记录大小
    void setSlotSize(uint16_t slot_num, uint16_t size); // 设置指定槽位的记录大小



    // 2. 页面元数据访问接口（与 cpp 中成员变量逻辑匹配）
    PageID getPageId() const { return header_.page_id; }          // 获取页面ID
    bool isDirty() const { return header_.is_dirty; }             // 判断是否为脏页
    void setDirty(bool dirty) { header_.is_dirty = dirty; }       // 设置脏页标记
    uint16_t getPinCount() const { return pin_count_; }           // 获取 pin 计数
    void incrementPinCount() { pin_count_++; }                    // 增加 pin 计数（cpp 中 fetchPage 会隐式调用）
    void decrementPinCount() { if (pin_count_ > 0) pin_count_--; } // 减少 pin 计数（防负）
    bool isPinned() const { return pin_count_ > 0; }              // 判断是否被 pin

    // 3. 记录操作接口（与 cpp 实现的参数、返回值、const 修饰完全匹配）
    bool insertRecord(const char* record_data, uint16_t record_size, RID* rid = nullptr);
    bool getRecord(const RID& rid, char* buffer, uint16_t* size = nullptr) const;
    bool deleteRecord(const RID& rid);
    bool updateRecord(const RID& rid, const char* new_data, uint16_t new_size, RID* new_rid = nullptr);

    // 4. 序列化/反序列化接口（与 cpp 中内存-磁盘交互逻辑匹配）
    void serialize(char* dest) const;  // 序列化：页面 → 字节缓冲区（供磁盘写入）
    void deserialize(const char* src); // 反序列化：字节缓冲区 → 页面（供磁盘读取）

    // 5. 页面信息与空间检查接口（与 cpp 辅助函数匹配）
    std::string toString() const;                  // 输出页面详细信息（调试用）
    bool hasEnoughSpace(uint16_t required) const;  // 检查是否有足够空间插入记录

    // 6. 数据区与页头访问接口（供外部模块如 DiskManager/BufferManager 使用）
    char* getData() { return data_; }                  // 获取数据区首地址（可修改）
    const char* getData() const { return data_; }      // 获取数据区首地址（只读）
    const PageHeader& getHeader() const { return header_; } // 获取页头（只读）
    PageHeader& getHeader() { return header_; }         // 获取页头（可修改）

    // 新增的方法
    uint16_t getFreeSpace() const { return header_.free_space; }
    PageType getPageType() const { return header_.page_type; }
    void setPageType(PageType type) { header_.page_type = type; }

    // 获取下一条记录（按槽位顺序）
    bool getNextRecord(RID& rid) const;

    // 在 storage/Page.h 里 Page 类中加
    void initAsDataPage() {
        header_.page_type = PageType::DATA_PAGE;
        header_.slot_count = 0;
        header_.free_space = PAGE_SIZE - sizeof(PageHeader);
        header_.free_space_offset = PAGE_SIZE - sizeof(PageHeader);
        header_.next_free_page = INVALID_PAGE_ID;
        header_.is_dirty = true;
        std::memset(data_, 0, sizeof(data_));
    }

    PageID getNextPageId() const {
        return header_.next_free_page;
    }

    void setNextPageId(PageID pid) {
        header_.next_free_page = pid;
        header_.is_dirty = true;
    }

    // uint32_t getFreeSpace() const {
    //     return header_.free_space;
    // }

private:
    // 7. 私有辅助函数（与 cpp 中槽位操作逻辑匹配，仅内部调用）
    uint16_t getSlotOffset(uint16_t slot_num) const;  // 获取指定槽位的记录偏移
    void setSlotOffset(uint16_t slot_num, uint16_t offset); // 设置指定槽位的记录偏移
     bool getSlotInfo(uint16_t slot_num, uint16_t* offset, uint16_t* size) const; // 获取槽位完整信息
    void compactify(); // 页面压缩（cpp 中为抛异常的空实现，后续可扩展）

    uint16_t getSlotCount(const Page& page) {
        return page.getHeader().slot_count;
    }
    // 8. 私有数据成员（与 cpp 中初始化、操作逻辑完全匹配）
    PageHeader header_;                                  // 页面头部（元数据）
    char data_[PAGE_SIZE - sizeof(PageHeader)];          // 页面数据区（存储记录+槽位信息）
    uint16_t pin_count_;                                 // 页面 pin 计数（防止被缓冲池淘汰）
};

} // namespace storage
} // namespace minidb

#endif // MINIDB_PAGE_H