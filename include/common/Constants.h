//
//常量定义
//


// include/common/Constants.h
#pragma once

namespace minidb {

    constexpr uint32_t PAGE_SIZE = 4096; // 4KB页大小
    constexpr uint32_t INVALID_PAGE_ID = UINT32_MAX;
    constexpr uint32_t CATALOG_PAGE_ID = 0; // 目录页的固定ID

} // namespace minidb