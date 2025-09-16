#pragma once
#include <cstdint>
#include <string>

namespace minidb {
    using PageID=int32_t;

    //定义页面ID类型
    using PageID=int32_t;

    // 支持的数据库类型枚举
    enum class TypeId {
        INVALID,    // 无效类型
        BOOLEAN,    // 布尔类型
        INTEGER,    // 整数类型（主要使用这个）
        VARCHAR,    // 字符串类型
        FLOAT
    };

    // 获取类型名称的辅助函数
    const char* getTypeName(TypeId type);

    // 记录标识符 (Record Identifier) - 用于定位磁盘上的记录
    struct RID {
        PageID page_id{-1};     // 页号，-1表示无效
        int32_t slot_num{-1};    // 槽位号，-1表示无效

        // 重载比较运算符
        bool operator==(const RID& other) const {
            return page_id == other.page_id && slot_num == other.slot_num;
        }

        bool operator<(const RID& other) const {
            if (page_id != other.page_id) return page_id < other.page_id;
            return slot_num < other.slot_num;
        }

        // 转换为字符串表示（用于调试）
        std::string toString() const {
            return "(" + std::to_string(page_id) + ", " + std::to_string(slot_num) + ")";
        }

        // 检查是否是有效的RID
        bool isValid() const {
            return page_id >= 0 && slot_num >= 0;
        }

        // 静态方法：获取无效的RID
        static RID invalid() { return RID{-1, -1}; }
    };

} // namespace minidb