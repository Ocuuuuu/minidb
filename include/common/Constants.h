#pragma once
#include "Types.h"

namespace minidb {

    // 系统常量配置


    // 页面大小（4KB是数据库系统的常见选择）
    constexpr int PAGE_SIZE = 4096;

    // 文件头大小（存储 page_count_ 和 free_list_head_）
    constexpr size_t FILE_HEADER_SIZE = sizeof(PageID) * 2;

    // 缓冲区默认大小（缓存1024个页面 ≈ 4MB）
    constexpr int DEFAULT_BUFFER_POOL_SIZE = 1024;

    // 系统预留页面ID
    constexpr int INVALID_PAGE_ID = -1;      // 无效页面ID
    constexpr int HEADER_PAGE_ID = 0;        // 元数据头页面

    // 数据类型限制
    constexpr int MAX_VARCHAR_LENGTH = 255;  // 字符串最大长度

    // 系统配置
    constexpr const char* DEFAULT_DB_NAME = "minidb";
    constexpr const char* DB_FILE_EXTENSION = ".mdb";

} // namespace minidb