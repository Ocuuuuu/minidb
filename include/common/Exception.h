#pragma once

#include <stdexcept>
#include <string>

#include "Types.h"

namespace minidb {

    // 基础数据库异常类
    class DatabaseException : public std::runtime_error {
    public:
        explicit DatabaseException(const std::string& msg)
            : std::runtime_error("[Database] " + msg) {}
    };

    // 类型不匹配异常
    class TypeMismatchException : public DatabaseException {
    public:
        explicit TypeMismatchException(const std::string& msg)
            : DatabaseException("Type mismatch: " + msg) {}
    };

    // 超出范围异常
    class OutOfRangeException : public DatabaseException {
    public:
        explicit OutOfRangeException(const std::string& msg)
            : DatabaseException("Out of range: " + msg) {}
    };

    // 空值异常
    class NullValueException : public DatabaseException {
    public:
        explicit NullValueException(const std::string& msg)
            : DatabaseException("Null value: " + msg) {}
    };

    // 缓冲池通用异常
    class BufferPoolException : public DatabaseException {
    public:
        explicit BufferPoolException(const std::string& msg)
            : DatabaseException("Buffer Pool Error: " + msg) {}
    };

    // 缓冲池已满异常
    class BufferPoolFullException : public BufferPoolException {
    public:
        explicit BufferPoolFullException(const std::string& msg)
            : BufferPoolException("Buffer pool full: " + msg) {}
    };

    // 页面不在缓冲池异常
    class PageNotInPoolException : public BufferPoolException {
    public:
        explicit PageNotInPoolException(PageID page_id)
            : BufferPoolException("Page not in buffer pool: " + std::to_string(page_id)) {}
    };

    // 页面被固定（无法淘汰/移除）异常
    class PinnedPageException : public BufferPoolException {
    public:
        explicit PinnedPageException(PageID page_id)
            : BufferPoolException("Page is pinned and cannot be evicted/removed: " + std::to_string(page_id)) {}
    };

    // 磁盘操作异常基类
    class DiskException : public DatabaseException {
    public:
        explicit DiskException(const std::string& msg)
            : DatabaseException("Disk Error: " + msg) {}
    };

    // 页面ID超出范围异常
    class PageOutOfRangeException : public DiskException {
    public:
        explicit PageOutOfRangeException(PageID page_id)
            : DiskException("Page ID out of range: " + std::to_string(page_id)) {}
    };

    // 磁盘IO异常
    class IOException : public DiskException {
    public:
        explicit IOException(const std::string& msg)
            : DiskException("IO Error: " + msg) {}
    };

    // 文件操作异常基类
    class FileException : public DatabaseException {
    public:
        explicit FileException(const std::string& msg)
            : DatabaseException("File Error: " + msg) {}
    };

    // 文件未找到异常
    class FileNotFoundException : public FileException {
    public:
        explicit FileNotFoundException(const std::string& path)
            : FileException("File not found: " + path) {}
    };

    // 目录创建失败异常
    class DirectoryCreationException : public FileException {
    public:
        explicit DirectoryCreationException(const std::string& path)
            : FileException("Cannot create directory: " + path) {}
    };

    // 数据库未打开异常
    class DatabaseNotOpenException : public FileException {
    public:
        DatabaseNotOpenException()
            : FileException("Database is not open") {}

        // 添加这两个构造函数
        explicit DatabaseNotOpenException(const std::string& msg)
            : FileException("Database is not open: " + msg) {}

        explicit DatabaseNotOpenException(const char* msg)
            : FileException(std::string("Database is not open: ") + msg) {}
    };

    // ---------------------- 页面相关异常 ----------------------
    class PageException : public DatabaseException {
    public:
        explicit PageException(const std::string& msg)
            : DatabaseException("Page Error: " + msg) {}
    };


    // 槽位号超出范围（页面内的槽位操作）
    class SlotOutOfRangeException : public PageException {
    public:
        explicit SlotOutOfRangeException(uint16_t slot_num, PageID page_id)
            : PageException("Slot " + std::to_string(slot_num) + " out of range (page " + std::to_string(page_id) + ")") {}
    };

    // RID无效（页ID不匹配、槽位号超界）
    class InvalidRIDException : public PageException {
    public:
        explicit InvalidRIDException(const RID& rid)
            : PageException("Invalid RID: PageID=" + std::to_string(rid.page_id) + ", SlotNum=" + std::to_string(rid.slot_num)) {}
    };

    // 记录插入失败（空间不足）
    class RecordInsertException : public PageException {
    public:
        explicit RecordInsertException(PageID page_id, uint16_t required, uint16_t available)
            : PageException("Insert record failed (page " + std::to_string(page_id) + "): Required=" + std::to_string(required) + ", Available=" + std::to_string(available)) {}
    };

    // 未实现功能异常（用于页面压缩等暂未实现的方法）
    class NotImplementedException : public DatabaseException {
    public:
        explicit NotImplementedException(const std::string& msg)
            : DatabaseException("Not implemented: " + msg) {}
    };

    // 无效槽位异常（槽位已被删除或不存在有效记录）
    class InvalidSlotException : public PageException {
    public:
        explicit InvalidSlotException(const std::string& msg)
            : PageException("Invalid slot: " + msg) {}
    };

} // namespace minidb