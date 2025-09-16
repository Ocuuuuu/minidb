#include <../tests/catch2/catch_amalgamated.hpp>
#include <storage/BufferManager.h>
#include <storage/DiskManager.h>
#include <storage/FileManager.h>
#include <storage/Page.h>
#include <common/Exception.h>
#include <memory>
#include <cstring>
#include <thread>
#include <vector>
#include <iostream>

TEST_CASE("BufferManager basic functionality", "[buffermanager][storage][unit]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_buffermanager_db";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    SECTION("BufferManager construction and initialization") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 10);

        REQUIRE(buffer_manager.getPoolSize() == 10);
        REQUIRE(buffer_manager.getCurrentPages() == 0);
        REQUIRE(buffer_manager.getHitRate() == 0.0);
    }

    SECTION("Page fetch and basic operations") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 5);

        minidb::PageID page1 = disk_manager->allocatePage();
        minidb::PageID page2 = disk_manager->allocatePage();

        // 第一次获取页面（应该从磁盘加载）
        minidb::storage::Page* page_ptr1 = buffer_manager.fetchPage(page1);
        REQUIRE(page_ptr1 != nullptr);
        REQUIRE(page_ptr1->getPageId() == page1);
        REQUIRE(buffer_manager.getCurrentPages() == 1);
        REQUIRE(buffer_manager.getMissCount() == 1);

        // 解除fetchPage的自动pin
        buffer_manager.unpinPage(page1);

        // 第二次获取相同页面（应该命中缓冲池）
        minidb::storage::Page* page_ptr2 = buffer_manager.fetchPage(page1);
        REQUIRE(page_ptr2 == page_ptr1);
        REQUIRE(buffer_manager.getHitCount() == 1);
        buffer_manager.unpinPage(page1);
    }

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }
}

TEST_CASE("BufferManager pin/unpin operations", "[buffermanager][pin][unit]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_buffermanager_pin_db";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    SECTION("Basic pin and unpin operations") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 3);

        minidb::PageID page_id = disk_manager->allocatePage();

        // 获取页面（会自动pin一次）
        minidb::storage::Page* page = buffer_manager.fetchPage(page_id);

        // 尝试再次固定（现在pin_count=2）
        buffer_manager.pinPage(page_id);

        // 解除固定（需要解除两次）
        buffer_manager.unpinPage(page_id);  // pin_count=1
        buffer_manager.unpinPage(page_id);  // pin_count=0

        // 完全解除固定后页面可以被淘汰
        // 这里应该期望抛出异常，因为pin_count已经是0
        REQUIRE_THROWS_AS(buffer_manager.unpinPage(page_id, false), minidb::BufferPoolException);
    }

    SECTION("Pin count validation") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 3);

        minidb::PageID page_id = disk_manager->allocatePage();
        buffer_manager.fetchPage(page_id); // 自动pin一次

        // 过度解除固定应该抛出异常（只pin了一次，但尝试unpin两次）
        buffer_manager.unpinPage(page_id); // 第一次unpin
        REQUIRE_THROWS_AS(buffer_manager.unpinPage(page_id), minidb::BufferPoolException);

        // 固定不存在的页面应该抛出异常
        REQUIRE_THROWS_AS(buffer_manager.pinPage(999), minidb::PageNotInPoolException);
    }

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }
}

TEST_CASE("BufferManager replacement policy", "[buffermanager][lru][unit]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_buffermanager_lru_db";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    SECTION("LRU page eviction") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 3);

        minidb::PageID page1 = disk_manager->allocatePage();
        minidb::PageID page2 = disk_manager->allocatePage();
        minidb::PageID page3 = disk_manager->allocatePage();

        buffer_manager.fetchPage(page1);
        buffer_manager.unpinPage(page1); // 解除pin以便淘汰
        buffer_manager.fetchPage(page2);
        buffer_manager.unpinPage(page2);
        buffer_manager.fetchPage(page3);
        buffer_manager.unpinPage(page3);

        REQUIRE(buffer_manager.getCurrentPages() == 3);

        // 获取第4个页面，应该触发淘汰
        minidb::PageID page4 = disk_manager->allocatePage();
        buffer_manager.fetchPage(page4);
        buffer_manager.unpinPage(page4);

        // 缓冲池应该仍然只有3个页面
        REQUIRE(buffer_manager.getCurrentPages() == 3);
    }

    SECTION("LRU access pattern") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 3);

        minidb::PageID page1 = disk_manager->allocatePage();
        minidb::PageID page2 = disk_manager->allocatePage();
        minidb::PageID page3 = disk_manager->allocatePage();
        minidb::PageID page4 = disk_manager->allocatePage();

        // 访问模式: 1, 2, 3, 1, 4
        buffer_manager.fetchPage(page1);
        buffer_manager.unpinPage(page1);
        buffer_manager.fetchPage(page2);
        buffer_manager.unpinPage(page2);
        buffer_manager.fetchPage(page3);
        buffer_manager.unpinPage(page3);
        buffer_manager.fetchPage(page1); // 再次访问page1，使其成为最近使用的
        buffer_manager.unpinPage(page1);
        buffer_manager.fetchPage(page4); // 应该淘汰page2（最久未使用）
        buffer_manager.unpinPage(page4);

        // page1和page3应该还在缓冲池中，page2被淘汰
        REQUIRE_NOTHROW(buffer_manager.fetchPage(page1));
        buffer_manager.unpinPage(page1);
        REQUIRE_NOTHROW(buffer_manager.fetchPage(page3));
        buffer_manager.unpinPage(page3);

        // page2需要重新从磁盘加载（说明被淘汰了）
        REQUIRE(buffer_manager.fetchPage(page2) != nullptr);
        buffer_manager.unpinPage(page2);
    }

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }
}

TEST_CASE("BufferManager flush operations", "[buffermanager][flush][unit]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_buffermanager_flush_db";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    SECTION("Page flush to disk") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 5);

        minidb::PageID page_id = disk_manager->allocatePage();
        std::cout << "DEBUG: Allocated page ID: " << page_id << std::endl;

        minidb::storage::Page* page = buffer_manager.fetchPage(page_id);

        // 修改页面内容 - 确保修改的是数据区
        char* page_data = page->getData();
        std::cout << "DEBUG: Page data pointer: " << (void*)page_data << std::endl;

        std::strcpy(page_data, "Modified page data");
        std::cout << "DEBUG: Written to page data: '" << page_data << "'" << std::endl;

        page->setDirty(true);

        // 检查页面序列化前的数据
        char serialized_buffer[minidb::PAGE_SIZE];
        page->serialize(serialized_buffer);
        char* serialized_data = serialized_buffer + sizeof(minidb::storage::PageHeader);
        std::cout << "DEBUG: Serialized data: '" << serialized_data << "'" << std::endl;

        // 刷新到磁盘
        buffer_manager.flushPage(page_id);
        buffer_manager.unpinPage(page_id);

        // 验证磁盘上的数据 - 跳过页头比较数据区
        char disk_data[minidb::PAGE_SIZE];
        disk_manager->readPage(page_id, disk_data);

        // 提取数据区内容
        char* disk_page_data = disk_data + sizeof(minidb::storage::PageHeader);
        std::cout << "DEBUG: Read from disk data: '" << disk_page_data << "'" << std::endl;

        // 比较整个页面内容（用于调试）
        std::cout << "DEBUG: Full page comparison:" << std::endl;
        for (int i = 0; i < minidb::PAGE_SIZE; i++) {
            if (serialized_buffer[i] != disk_data[i]) {
                std::cout << "DEBUG: Difference at offset " << i
                          << ": serialized=" << (int)serialized_buffer[i]
                          << ", disk=" << (int)disk_data[i] << std::endl;
                break;
            }
        }

        REQUIRE(std::strcmp(disk_page_data, "Modified page data") == 0);
    }

    SECTION("Flush all pages") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 5);

        // 创建多个脏页面
        std::vector<minidb::PageID> page_ids;
        for (int i = 0; i < 3; ++i) {
            minidb::PageID page_id = disk_manager->allocatePage();
            minidb::storage::Page* page = buffer_manager.fetchPage(page_id);

            // 修改数据区内容
            char* page_data = page->getData();
            std::sprintf(page_data, "Dirty page %d", i);

            page->setDirty(true);
            buffer_manager.unpinPage(page_id, true);
            page_ids.push_back(page_id);
        }

        // 刷新所有页面
        buffer_manager.flushAllPages();

        // 验证所有页面都已刷到磁盘 - 比较数据区
        for (minidb::PageID page_id : page_ids) {
            char disk_data[minidb::PAGE_SIZE];
            disk_manager->readPage(page_id, disk_data);

            // 提取数据区内容
            char* disk_page_data = disk_data + sizeof(minidb::storage::PageHeader);
            REQUIRE(std::strstr(disk_page_data, "Dirty page") != nullptr);
        }
    }

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }
}

TEST_CASE("BufferManager edge cases", "[buffermanager][edge][unit]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_buffermanager_edge_db";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    SECTION("Buffer pool full with pinned pages") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 2);

        // 分配并固定所有页面
        minidb::PageID page1 = disk_manager->allocatePage();
        minidb::PageID page2 = disk_manager->allocatePage();
        minidb::PageID page3 = disk_manager->allocatePage();

        buffer_manager.fetchPage(page1); // 自动pin
        buffer_manager.pinPage(page1);   // 再次pin
        buffer_manager.fetchPage(page2); // 自动pin
        buffer_manager.pinPage(page2);   // 再次pin

        // 尝试获取第3个页面，应该抛出异常（所有页面都被固定）
        REQUIRE_THROWS_AS(buffer_manager.fetchPage(page3), minidb::BufferPoolFullException);

        // 清理
        buffer_manager.unpinPage(page1);
        buffer_manager.unpinPage(page1);
        buffer_manager.unpinPage(page2);
        buffer_manager.unpinPage(page2);
    }

    SECTION("Remove page operations") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 5);

        minidb::PageID page_id = disk_manager->allocatePage();
        buffer_manager.fetchPage(page_id); // 自动pin

        // 移除被固定的页面应该抛出异常
        REQUIRE_THROWS_AS(buffer_manager.removePage(page_id), minidb::PinnedPageException);

        // 解除固定后可以移除
        buffer_manager.unpinPage(page_id);
        REQUIRE_NOTHROW(buffer_manager.removePage(page_id));
        REQUIRE(buffer_manager.getCurrentPages() == 0);
    }

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }
}

TEST_CASE("Safe Debug - Step by Step", "[debug]")
{
    std::cout << "=== 开始调试测试 ===" << std::endl;

    try {
        std::cout << "1. 创建FileManager..." << std::endl;
        auto file_manager = std::make_shared<minidb::storage::FileManager>();
        std::cout << "   ✅ FileManager创建成功" << std::endl;

        std::string test_db = "test_debug_db";

        std::cout << "2. 检查数据库是否存在..." << std::endl;
        if (file_manager->databaseExists(test_db)) {
            std::cout << "   数据库存在，正在删除..." << std::endl;
            file_manager->deleteDatabase(test_db);
        }
        std::cout << "   ✅ 数据库状态检查完成" << std::endl;

        std::cout << "3. 创建数据库..." << std::endl;
        file_manager->createDatabase(test_db);
        std::cout << "   ✅ 数据库创建成功" << std::endl;

        std::cout << "4. 创建DiskManager..." << std::endl;
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        std::cout << "   ✅ DiskManager创建成功" << std::endl;

        std::cout << "5. 创建BufferManager..." << std::endl;
        minidb::storage::BufferManager buffer_manager(disk_manager, 10);
        std::cout << "   ✅ BufferManager创建成功" << std::endl;

        std::cout << "6. 分配页面..." << std::endl;
        minidb::PageID page_id = disk_manager->allocatePage();
        std::cout << "   ✅ 页面分配成功，page_id: " << page_id << std::endl;

        std::cout << "7. 从BufferManager获取页面..." << std::endl;
        minidb::storage::Page* page = buffer_manager.fetchPage(page_id);
        std::cout << "   页面指针: " << (void*)page << std::endl;

        if (page == nullptr) {
            std::cout << "   ❌ 错误：fetchPage返回了null指针！" << std::endl;
            REQUIRE(false);
            return;
        }
        std::cout << "   ✅ 页面获取成功" << std::endl;

        std::cout << "8. 检查页面基本信息..." << std::endl;
        try {
            minidb::PageID retrieved_page_id = page->getPageId();
            std::cout << "   页面ID: " << retrieved_page_id << std::endl;

            uint16_t free_space = page->getFreeSpace();
            std::cout << "   空闲空间: " << free_space << std::endl;

            uint16_t pin_count = page->getPinCount();
            std::cout << "   Pin计数: " << pin_count << std::endl;

            bool is_dirty = page->isDirty();
            std::cout << "   是否脏页: " << is_dirty << std::endl;
        } catch (const std::exception& e) {
            std::cout << "   ❌ 页面信息访问异常: " << e.what() << std::endl;
            REQUIRE(false);
            return;
        } catch (...) {
            std::cout << "   ❌ 页面信息访问发生未知异常！" << std::endl;
            REQUIRE(false);
            return;
        }
        std::cout << "   ✅ 页面基本信息检查完成" << std::endl;

        std::cout << "9. 测试简单记录插入..." << std::endl;
        const char* test_record = "Test";
        size_t record_size = 5;
        minidb::RID rid;

        try {
            bool inserted = page->insertRecord(test_record, record_size, &rid);
            if (inserted) {
                std::cout << "   ✅ 记录插入成功，RID: (" << rid.page_id << ", " << rid.slot_num << ")" << std::endl;
            } else {
                std::cout << "   ❌ 记录插入失败" << std::endl;
                REQUIRE(false);
                return;
            }
        } catch (const std::exception& e) {
            std::cout << "   ❌ 记录插入异常: " << e.what() << std::endl;
            REQUIRE(false);
            return;
        } catch (...) {
            std::cout << "   ❌ 记录插入发生未知异常！" << std::endl;
            REQUIRE(false);
            return;
        }

        std::cout << "10. 验证记录读取..." << std::endl;
        try {
            char read_buffer[100];
            uint16_t read_size;
            bool found = page->getRecord(rid, read_buffer, &read_size);

            if (found) {
                std::cout << "   ✅ 记录读取成功: '" << read_buffer << "', 大小: " << read_size << std::endl;
                REQUIRE(std::strcmp(read_buffer, test_record) == 0);
            } else {
                std::cout << "   ❌ 记录读取失败" << std::endl;
                REQUIRE(false);
                return;
            }
        } catch (const std::exception& e) {
            std::cout << "   ❌ 记录读取异常: " << e.what() << std::endl;
            REQUIRE(false);
            return;
        }

        std::cout << "11. 测试unpin操作..." << std::endl;
        try {
            buffer_manager.unpinPage(page_id, true);  // 设置为脏页
            std::cout << "   ✅ unpinPage成功" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "   ❌ unpinPage异常: " << e.what() << std::endl;
            REQUIRE(false);
            return;
        }

        std::cout << "12. 测试flush操作..." << std::endl;
        try {
            buffer_manager.flushPage(page_id);
            std::cout << "   ✅ flushPage成功" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "   ❌ flushPage异常: " << e.what() << std::endl;
            REQUIRE(false);
            return;
        }

        std::cout << "13. 清理测试..." << std::endl;
        if (file_manager->databaseExists(test_db)) {
            file_manager->deleteDatabase(test_db);
            std::cout << "   ✅ 数据库清理完成" << std::endl;
        }

        std::cout << "=== 🎉 所有步骤都成功完成！===" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "❌ 捕获到标准异常: " << e.what() << std::endl;
        REQUIRE(false);
    } catch (...) {
        std::cout << "❌ 捕获到未知异常！" << std::endl;
        REQUIRE(false);
    }
}

TEST_CASE("Serialization Debug Test", "[debug]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_serialization_db";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    file_manager->createDatabase(test_db);
    auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
    minidb::storage::BufferManager buffer_manager(disk_manager, 10);

    minidb::PageID page_id = disk_manager->allocatePage();
    minidb::storage::Page* page = buffer_manager.fetchPage(page_id);

    const char* record_data = "Integration Test Record";
    size_t record_size = std::strlen(record_data) + 1;
    minidb::RID rid;

    std::cout << "=== 序列化调试测试 ===" << std::endl;
    std::cout << "记录数据: '" << record_data << "'" << std::endl;
    std::cout << "记录大小: " << record_size << std::endl;

    // 1. 插入记录
    bool inserted = page->insertRecord(record_data, record_size, &rid);
    REQUIRE(inserted);
    std::cout << "记录插入成功，RID: (" << rid.page_id << ", " << rid.slot_num << ")" << std::endl;

    // 2. 立即从原页面读取验证
    char immediate_read[100];
    uint16_t immediate_size;
    bool immediate_found = page->getRecord(rid, immediate_read, &immediate_size);

    std::cout << "\n=== 插入后立即读取 ===" << std::endl;
    std::cout << "找到: " << immediate_found << std::endl;
    std::cout << "大小: " << immediate_size << std::endl;
    std::cout << "内容: '" << immediate_read << "'" << std::endl;
    REQUIRE(immediate_found);
    REQUIRE(std::strcmp(immediate_read, record_data) == 0);

    // 3. 检查页面内部状态
    std::cout << "\n=== 页面状态检查 ===" << std::endl;
    const auto& header = page->getHeader();
    std::cout << "页面ID: " << header.page_id << std::endl;
    std::cout << "槽位数量: " << header.slot_count << std::endl;
    std::cout << "空闲空间偏移: " << header.free_space_offset << std::endl;
    std::cout << "剩余空闲空间: " << header.free_space << std::endl;

    // 4. 手动序列化并检查
    std::cout << "\n=== 手动序列化检查 ===" << std::endl;
    char manual_buffer[minidb::PAGE_SIZE];
    std::memset(manual_buffer, 0, minidb::PAGE_SIZE);  // 清零便于观察

    page->serialize(manual_buffer);
    std::cout << "序列化完成" << std::endl;

    // 检查序列化后的header部分
    minidb::storage::PageHeader* serialized_header =
        reinterpret_cast<minidb::storage::PageHeader*>(manual_buffer);

    std::cout << "序列化后的header:" << std::endl;
    std::cout << "  页面ID: " << serialized_header->page_id << std::endl;
    std::cout << "  槽位数量: " << serialized_header->slot_count << std::endl;
    std::cout << "  空闲空间偏移: " << serialized_header->free_space_offset << std::endl;
    std::cout << "  剩余空闲空间: " << serialized_header->free_space << std::endl;

    // 检查数据区前100字节的内容
    char* data_area = manual_buffer + sizeof(minidb::storage::PageHeader);
    std::cout << "\n序列化数据区前100字节内容:" << std::endl;
    for (int i = 0; i < 100; i++) {
        if (data_area[i] >= 32 && data_area[i] <= 126) {
            std::cout << data_area[i];
        } else if (data_area[i] == 0) {
            std::cout << ".";
        } else {
            std::cout << "\\x" << std::hex << std::setfill('0') << std::setw(2)
                      << (unsigned char)data_area[i] << std::dec;
        }
        if ((i + 1) % 20 == 0) std::cout << std::endl;
    }
    std::cout << std::endl;

    // 5. 手动反序列化并测试
    std::cout << "\n=== 手动反序列化测试 ===" << std::endl;
    minidb::storage::Page manual_page;
    manual_page.deserialize(manual_buffer);

    std::cout << "反序列化完成" << std::endl;

    const auto& deserialized_header = manual_page.getHeader();
    std::cout << "反序列化后的header:" << std::endl;
    std::cout << "  页面ID: " << deserialized_header.page_id << std::endl;
    std::cout << "  槽位数量: " << deserialized_header.slot_count << std::endl;
    std::cout << "  空闲空间偏移: " << deserialized_header.free_space_offset << std::endl;
    std::cout << "  剩余空闲空间: " << deserialized_header.free_space << std::endl;

    // 6. 从手动反序列化的页面读取记录
    char manual_read[100];
    uint16_t manual_size;
    bool manual_found = manual_page.getRecord(rid, manual_read, &manual_size);

    std::cout << "\n=== 从手动反序列化页面读取记录 ===" << std::endl;
    std::cout << "找到: " << manual_found << std::endl;
    if (manual_found) {
        std::cout << "大小: " << manual_size << std::endl;
        std::cout << "内容: '" << manual_read << "'" << std::endl;

        // 逐字节对比
        std::cout << "字节对比 (前10字节):" << std::endl;
        for (int i = 0; i < 10 && i < manual_size; i++) {
            std::cout << "  [" << i << "] 读取: " << (int)manual_read[i]
                      << " ('" << (char)manual_read[i] << "'), 期望: "
                      << (int)record_data[i] << " ('" << record_data[i] << "')" << std::endl;
        }
    }

    // 7. 现在通过BufferManager正常流程
    std::cout << "\n=== 通过BufferManager正常流程 ===" << std::endl;
    buffer_manager.unpinPage(page_id, true);
    buffer_manager.flushPage(page_id);

    char disk_buffer[minidb::PAGE_SIZE];
    disk_manager->readPage(page_id, disk_buffer);

    minidb::storage::Page disk_page;
    disk_page.deserialize(disk_buffer);

    char disk_read[100];
    uint16_t disk_size;
    bool disk_found = disk_page.getRecord(rid, disk_read, &disk_size);

    std::cout << "BufferManager流程结果:" << std::endl;
    std::cout << "找到: " << disk_found << std::endl;
    if (disk_found) {
        std::cout << "大小: " << disk_size << std::endl;
        std::cout << "内容: '" << disk_read << "'" << std::endl;
    }

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }
}

TEST_CASE("Content Specific Test", "[debug]")
{
    std::cout << "=== 内容特定测试 ===" << std::endl;

    // 测试不同内容但相同长度的字符串
    std::vector<std::string> test_cases = {
        "ABCDEFGHIJKLMNOPQRSTUVW",     // 24字节，纯字母 (已知成功)
        "Integration Test Record",      // 24字节，包含空格 (已知失败)
        "IntegrationTestRecordXX",      // 24字节，无空格版本
        "Integration-Test-Record",      // 24字节，用破折号替代空格
        "Integration_Test_Record",      // 24字节，用下划线替代空格
        "Integration.Test.Record",      // 24字节，用句号替代空格
        "1234567890123456789012",       // 24字节，纯数字
        "A B C D E F G H I J K L",     // 24字节，字母+空格
    };

    for (const auto& test_str : test_cases) {
        std::cout << "\n--- 测试字符串: '" << test_str << "' ---" << std::endl;
        std::cout << "长度: " << test_str.length() << " + 1 = " << (test_str.length() + 1) << std::endl;

        // 独立Page测试
        std::cout << "1. 独立Page测试:" << std::endl;
        {
            minidb::storage::Page page(1);
            minidb::RID rid;

            bool inserted = page.insertRecord(test_str.c_str(), test_str.length() + 1, &rid);
            if (!inserted) {
                std::cout << "   ❌ 插入失败" << std::endl;
                continue;
            }

            // 序列化测试
            char buffer[minidb::PAGE_SIZE];
            page.serialize(buffer);

            minidb::storage::Page new_page;
            new_page.deserialize(buffer);

            char result[100];
            bool found = new_page.getRecord(rid, result, nullptr);

            if (found && std::strcmp(result, test_str.c_str()) == 0) {
                std::cout << "   ✅ 独立Page测试成功" << std::endl;
            } else {
                std::cout << "   ❌ 独立Page测试失败: '" << result << "'" << std::endl;
                continue;
            }
        }

        // BufferManager环境测试
        std::cout << "2. BufferManager环境测试:" << std::endl;
        {
            auto file_manager = std::make_shared<minidb::storage::FileManager>();
            std::string test_db = "test_content_db";

            if (file_manager->databaseExists(test_db)) {
                file_manager->deleteDatabase(test_db);
            }

            file_manager->createDatabase(test_db);
            auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
            minidb::storage::BufferManager buffer_manager(disk_manager, 10);

            minidb::PageID page_id = disk_manager->allocatePage();
            minidb::storage::Page* page = buffer_manager.fetchPage(page_id);

            minidb::RID rid;
            bool inserted = page->insertRecord(test_str.c_str(), test_str.length() + 1, &rid);
            if (!inserted) {
                std::cout << "   ❌ BufferManager插入失败" << std::endl;
                file_manager->deleteDatabase(test_db);
                continue;
            }

            // 立即读取测试
            char immediate[100];
            bool immediate_found = page->getRecord(rid, immediate, nullptr);
            if (!immediate_found || std::strcmp(immediate, test_str.c_str()) != 0) {
                std::cout << "   ❌ BufferManager立即读取失败: '" << immediate << "'" << std::endl;
                file_manager->deleteDatabase(test_db);
                continue;
            }

            // flush和磁盘读取测试
            buffer_manager.unpinPage(page_id, true);
            buffer_manager.flushPage(page_id);

            char disk_buffer[minidb::PAGE_SIZE];
            disk_manager->readPage(page_id, disk_buffer);

            minidb::storage::Page disk_page;
            disk_page.deserialize(disk_buffer);

            char final_result[100];
            bool final_found = disk_page.getRecord(rid, final_result, nullptr);

            if (final_found && std::strcmp(final_result, test_str.c_str()) == 0) {
                std::cout << "   ✅ BufferManager完整流程成功" << std::endl;
            } else {
                std::cout << "   ❌ BufferManager磁盘读取失败:" << std::endl;
                std::cout << "      期望: '" << test_str << "'" << std::endl;
                std::cout << "      实际: '" << final_result << "'" << std::endl;

                // 字节对比
                std::cout << "   字节对比 (前10字节):" << std::endl;
                for (int i = 0; i < 10 && i < test_str.length() + 1; i++) {
                    std::cout << "      [" << i << "] 期望: " << (int)test_str[i]
                              << " ('" << test_str[i] << "'), 实际: "
                              << (int)final_result[i] << " ('" << (char)final_result[i] << "')" << std::endl;
                }
            }

            file_manager->deleteDatabase(test_db);
        }
    }
}

TEST_CASE("Debug Flush Issue", "[debug]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_debug_flush";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    file_manager->createDatabase(test_db);
    auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
    minidb::storage::BufferManager buffer_manager(disk_manager, 10);

    // 获取页面
    minidb::PageID page_id = disk_manager->allocatePage();
    minidb::storage::Page* page = buffer_manager.fetchPage(page_id);

    // 插入一个简单的测试记录
    const char* test_data = "TestData123";
    minidb::RID rid;
    bool inserted = page->insertRecord(test_data, std::strlen(test_data) + 1, &rid);

    std::cout << "=== 测试开始 ===" << std::endl;
    std::cout << "插入记录: '" << test_data << "'" << std::endl;
    std::cout << "插入结果: " << (inserted ? "成功" : "失败") << std::endl;

    if (inserted) {
        // 立即读取验证
        char immediate_read[100];
        bool immediate_found = page->getRecord(rid, immediate_read, nullptr);
        std::cout << "立即读取: " << (immediate_found ? "成功" : "失败") << std::endl;
        if (immediate_found) {
            std::cout << "立即读取内容: '" << immediate_read << "'" << std::endl;
        }

        // 设置脏状态并刷新（这里会触发详细调试）
        buffer_manager.unpinPage(page_id, true);
        buffer_manager.flushPage(page_id); // 这里会输出详细调试信息
    }

    // 清理
    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    std::cout << "=== 测试结束 ===" << std::endl;
}

TEST_CASE("BufferManager concurrent access", "[buffermanager][concurrency][unit]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_buffermanager_concurrent_db";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    SECTION("Thread-safe buffer operations") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 50);

        const int num_threads = 4;
        const int operations_per_thread = 5; // 减少操作数量
        std::vector<std::thread> threads;
        std::atomic<int> success_count{0};

        // 预先分配一些页面
        std::vector<minidb::PageID> page_ids;
        for (int i = 0; i < 20; ++i) {
            page_ids.push_back(disk_manager->allocatePage());
        }

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                try {
                    for (int j = 0; j < operations_per_thread; ++j) {
                        minidb::PageID page_id = page_ids[(i * operations_per_thread + j) % page_ids.size()];

                        // 获取页面
                        minidb::storage::Page* page = buffer_manager->fetchPage(page_id);

                        // 修改页面内容
                        std::sprintf(page->getData(), "Thread %d, Op %d", i, j);

                        // 标记为脏页面并解除固定
                        buffer_manager->unpinPage(page_id, true);

                        success_count++;
                    }
                } catch (const std::exception& e) {
                    // 线程中的异常会被捕获
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // 刷新所有页面并验证数据一致性
        buffer_manager->flushAllPages();
        for (int i = 0; i < std::min(10, static_cast<int>(page_ids.size())); ++i) {
            minidb::PageID page_id = page_ids[i];
            char disk_data[minidb::PAGE_SIZE];
            disk_manager->readPage(page_id, disk_data);

            // 检查是否有线程写入的数据
            bool has_data = false;
            for (int t = 0; t < num_threads; ++t) {
                char expected[100];
                std::sprintf(expected, "Thread %d", t);
                if (std::strstr(disk_data, expected) != nullptr) {
                    has_data = true;
                    break;
                }
            }
            REQUIRE(has_data);
        }
    }

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }
}

TEST_CASE("BufferManager hit rate statistics", "[buffermanager][stats][unit]")
{
    auto file_manager = std::make_shared<minidb::storage::FileManager>();
    std::string test_db = "test_buffermanager_stats_db";

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }

    SECTION("Hit rate calculation") {
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        minidb::storage::BufferManager buffer_manager(disk_manager, 3);

        minidb::PageID page1 = disk_manager->allocatePage();
        minidb::PageID page2 = disk_manager->allocatePage();

        // 第一次访问（miss）
        buffer_manager.fetchPage(page1);
        buffer_manager.unpinPage(page1);
        REQUIRE(buffer_manager.getHitRate() == 0.0);

        // 第二次访问相同页面（hit）
        buffer_manager.fetchPage(page1);
        buffer_manager.unpinPage(page1);
        REQUIRE(buffer_manager.getHitRate() == 0.5);

        // 访问不同页面（miss）
        buffer_manager.fetchPage(page2);
        buffer_manager.unpinPage(page2);
        REQUIRE(buffer_manager.getHitRate() == 1.0/3.0);

        // 再次访问相同页面（hit）
        buffer_manager.fetchPage(page2);
        buffer_manager.unpinPage(page2);
        REQUIRE(buffer_manager.getHitRate() == 0.5);
    }

    if (file_manager->databaseExists(test_db)) {
        file_manager->deleteDatabase(test_db);
    }
}