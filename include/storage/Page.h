//
// Created by tang_ on 2025/9/9.
//

#ifndef MINIDB_PAGE_H
#define MINIDB_PAGE_H


#include "common/Constants.h"
#include "common/Types.h"
#include "common/Tuple.h"
#include <cstring>

namespace minidb {
    namespace storage {

        // 页面头结构
        struct PageHeader {
            PageID page_id{INVALID_PAGE_ID};      // 页面ID
            uint16_t free_space_offset{0};        // 空闲空间起始偏移
            uint16_t slot_count{0};               // 槽位数量
            uint16_t free_space{0};               // 空闲空间大小
            bool is_dirty{false};                 // 脏页标志
        };

        class Page {
        public:
            Page();
            explicit Page(PageID page_id);

            // 基本信息
            PageID getPageId() const { return header_.page_id; }
            bool isDirty() const { return header_.is_dirty; }
            void setDirty(bool dirty) { header_.is_dirty = dirty; }

            // 数据访问
            char* getData() { return data_; }
            const char* getData() const { return data_; }
            PageHeader getHeader() const { return header_; }

            // 空间管理
            uint16_t getFreeSpace() const { return header_.free_space; }
            bool hasEnoughSpace(uint16_t required) const { return header_.free_space >= required; }

            // 记录操作
            bool insertRecord(const char* record_data, uint16_t record_size, RID* rid = nullptr);
            bool getRecord(const RID& rid, char* buffer, uint16_t* size = nullptr) const;
            bool deleteRecord(const RID& rid);
            bool updateRecord(const RID& rid, const char* new_data, uint16_t new_size);

            // 调试信息
            std::string toString() const;

            // 序列化和反序列化
            void serialize(char* dest) const;
            void deserialize(const char* src);

        private:
            PageHeader header_;
            char data_[PAGE_SIZE - sizeof(PageHeader)]; // 页面数据区域

            // 槽位操作辅助函数
            uint16_t getSlotOffset(uint16_t slot_num) const;
            void setSlotOffset(uint16_t slot_num, uint16_t offset);
            uint16_t getSlotSize(uint16_t slot_num) const;
            void setSlotSize(uint16_t slot_num, uint16_t size);
            bool getSlotInfo(uint16_t slot_num, uint16_t* offset, uint16_t* size) const;

            // 空间管理辅助函数
            void compactify();
        };

    } // namespace storage
} // namespace minidb
#endif //MINIDB_PAGE_H