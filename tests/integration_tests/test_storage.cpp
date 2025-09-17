#include <../tests/catch2/catch_amalgamated.hpp>
#include "storage/Pager.h"
#include "storage/DiskManager.h"
#include "storage/BufferManager.h"
#include "storage/FileManager.h"
#include "storage/Page.h"
#include "common/Exception.h"
#include <memory>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <thread>

TEST_CASE("Complete Persistence Integration Test", "[integration][persistence]")
{
    std::cout << "\n=== 开始集成测试：数据持久化验证 ===\n" << std::endl;

    // 测试数据库名称
    std::string test_db = "integration_test_db";

    // 第一阶段：数据写入和立即读取
    SECTION("Phase 1: Write and Immediate Read") {
        std::cout << "=== 第一阶段：数据写入和立即读取 ===" << std::endl;

        // 清理可能存在的旧测试文件
        if (std::filesystem::exists(test_db + ".minidb")) {
            std::filesystem::remove(test_db + ".minidb");
        }

        // 创建文件管理器
        auto file_manager = std::make_shared<minidb::storage::FileManager>();
        file_manager->createDatabase(test_db);

        // 创建磁盘管理器
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);

        // 创建缓冲管理器
        auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 20);

        // 创建页面管理器
        minidb::storage::Pager pager(disk_manager, buffer_manager);

        std::cout << "1. 分配新页面..." << std::endl;
        minidb::PageID page_id = pager.allocatePage();
        std::cout << "   分配的页面ID: " << page_id << std::endl;
        std::cout << "   磁盘位置: 偏移量 " << (page_id * minidb::PAGE_SIZE) << " 字节" << std::endl;

        // 获取页面
        std::cout << "2. 获取页面进行写入..." << std::endl;
        minidb::storage::Page* page = pager.getPage(page_id);
        REQUIRE(page != nullptr);

        // 插入测试数据
        std::cout << "3. 插入测试记录..." << std::endl;
        const char* test_records[] = {
            "First test record - Hello World!",
            "Second test record - MiniDB Storage",
            "Third test record - Persistence Test"
        };

        std::vector<minidb::RID> rids;
        for (const auto& record : test_records) {
            minidb::RID rid;
            bool inserted = page->insertRecord(record, std::strlen(record) + 1, &rid);
            REQUIRE(inserted);
            rids.push_back(rid);
            std::cout << "   插入记录: '" << record << "' -> RID("
                      << rid.page_id << ", " << rid.slot_num << ")" << std::endl;
        }

        // 立即读取验证
        std::cout << "4. 立即读取验证..." << std::endl;
        for (size_t i = 0; i < rids.size(); i++) {
            char buffer[100];
            uint16_t size;
            bool found = page->getRecord(rids[i], buffer, &size);
            REQUIRE(found);
            REQUIRE(std::strcmp(buffer, test_records[i]) == 0);
            std::cout << "   验证记录 " << i << ": '" << buffer << "' ✓" << std::endl;
        }

        // 释放页面并标记为脏
        std::cout << "5. 释放页面并刷新到磁盘..." << std::endl;
        pager.releasePage(page_id, true);
        pager.flushAll();

        std::cout << "6. 第一阶段完成，数据已写入磁盘" << std::endl;
        std::cout << "   文件大小: " << std::filesystem::file_size(test_db + ".minidb") << " 字节" << std::endl;
    }

    // 第二阶段：重启后读取验证
    SECTION("Phase 2: Read After Restart Simulation") {
        std::cout << "\n=== 第二阶段：模拟重启后读取 ===" << std::endl;
        std::cout << "模拟系统重启..." << std::endl;

        // 重新创建所有管理器（模拟重启）
        auto file_manager = std::make_shared<minidb::storage::FileManager>();
        file_manager->openDatabase(test_db);

        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 20);
        minidb::storage::Pager pager(disk_manager, buffer_manager);

        std::cout << "1. 重新打开数据库成功" << std::endl;
        std::cout << "   当前页面数量: " << pager.getPageCount() << std::endl;
        std::cout << "   文件大小: " << std::filesystem::file_size(test_db + ".minidb") << " 字节" << std::endl;

        // 假设我们知道之前分配的页面ID是1
        minidb::PageID page_id = 1;

        std::cout << "2. 获取页面ID " << page_id << " 进行读取..." << std::endl;
        minidb::storage::Page* page = pager.getPage(page_id);
        REQUIRE(page != nullptr);

        // 验证页面头信息
        const auto& header = page->getHeader();
        std::cout << "3. 页面头信息验证:" << std::endl;
        std::cout << "   页面ID: " << header.page_id << " ✓" << std::endl;
        std::cout << "   槽位数量: " << header.slot_count << " ✓" << std::endl;
        std::cout << "   空闲空间: " << header.free_space << " 字节" << std::endl;

        REQUIRE(header.page_id == page_id);
        REQUIRE(header.slot_count == 3); // 我们插入了3条记录

        // 读取并验证所有记录
        std::cout << "4. 读取并验证所有记录..." << std::endl;
        const char* expected_records[] = {
            "First test record - Hello World!",
            "Second test record - MiniDB Storage",
            "Third test record - Persistence Test"
        };

        for (uint16_t slot_num = 0; slot_num < header.slot_count; slot_num++) {
            minidb::RID rid{page_id, slot_num};
            char buffer[100];
            uint16_t size;

            bool found = page->getRecord(rid, buffer, &size);
            REQUIRE(found);
            REQUIRE(std::strcmp(buffer, expected_records[slot_num]) == 0);

            std::cout << "   记录 " << slot_num << ": '" << buffer << "' ✓" << std::endl;
            std::cout << "      RID: (" << rid.page_id << ", " << rid.slot_num << ")" << std::endl;
            std::cout << "      大小: " << size << " 字节" << std::endl;
        }

        // 释放页面
        pager.releasePage(page_id, false);

        std::cout << "5. 第二阶段完成，重启后数据读取验证成功 ✓" << std::endl;
    }

    // 第三阶段：额外操作和清理
    SECTION("Phase 3: Additional Operations and Cleanup") {
        std::cout << "\n=== 第三阶段：额外操作和清理 ===" << std::endl;

        auto file_manager = std::make_shared<minidb::storage::FileManager>();
        file_manager->openDatabase(test_db);

        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 20);
        minidb::storage::Pager pager(disk_manager, buffer_manager);

        // 测试分配新页面
        std::cout << "1. 测试分配新页面..." << std::endl;
        minidb::PageID new_page_id = pager.allocatePage();
        std::cout << "   新分配的页面ID: " << new_page_id << std::endl;
        std::cout << "   磁盘位置: 偏移量 " << (new_page_id * minidb::PAGE_SIZE) << " 字节" << std::endl;

        // 在新页面插入数据
        minidb::storage::Page* new_page = pager.getPage(new_page_id);
        const char* new_record = "Additional test record after restart";
        minidb::RID new_rid;

        bool inserted = new_page->insertRecord(new_record, std::strlen(new_record) + 1, &new_rid);
        REQUIRE(inserted);
        std::cout << "   在新页面插入记录: '" << new_record << "'" << std::endl;

        // 验证新记录
        char verify_buffer[100];
        bool found = new_page->getRecord(new_rid, verify_buffer, nullptr);
        REQUIRE(found);
        REQUIRE(std::strcmp(verify_buffer, new_record) == 0);
        std::cout << "   新记录验证成功 ✓" << std::endl;

        // 释放并刷新
        pager.releasePage(new_page_id, true);
        pager.flushAll();

        // 清理
        std::cout << "2. 清理测试文件..." << std::endl;
        file_manager->closeDatabase();

        if (std::filesystem::exists(test_db + ".minidb")) {
            std::filesystem::remove(test_db + ".minidb");
            std::cout << "   测试文件已清理 ✓" << std::endl;
        }

        std::cout << "3. 集成测试全部完成！数据持久化验证成功 ✓" << std::endl;
    }
}

TEST_CASE("Advanced Persistence Test with Multiple Pages", "[integration][advanced]")
{
    std::cout << "\n=== 高级集成测试：多页面操作 ===" << std::endl;

    std::string test_db = "advanced_test_db";

    // 清理旧文件
    if (std::filesystem::exists(test_db + ".minidb")) {
        std::filesystem::remove(test_db + ".minidb");
    }

    // 第一阶段：创建多页面数据库
    {
        auto file_manager = std::make_shared<minidb::storage::FileManager>();
        file_manager->createDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 50);
        minidb::storage::Pager pager(disk_manager, buffer_manager);

        std::cout << "创建5个页面并填充数据..." << std::endl;
        std::vector<minidb::PageID> page_ids;
        std::vector<std::vector<minidb::RID>> all_rids;

        // 创建5个页面
        for (int i = 0; i < 5; i++) {
            minidb::PageID page_id = pager.allocatePage();
            page_ids.push_back(page_id);
            std::cout << "页面 " << i << ": ID=" << page_id
                      << ", 偏移量=" << (page_id * minidb::PAGE_SIZE) << std::endl;

            minidb::storage::Page* page = pager.getPage(page_id);
            std::vector<minidb::RID> rids;

            // 每个页面插入3条记录
            for (int j = 0; j < 3; j++) {
                std::string record = "Page " + std::to_string(i) +
                                   ", Record " + std::to_string(j) +
                                   " - Test Data " + std::to_string(rand() % 1000);

                minidb::RID rid;
                bool inserted = page->insertRecord(record.c_str(), record.size() + 1, &rid);
                REQUIRE(inserted);
                rids.push_back(rid);
            }

            all_rids.push_back(rids);
            pager.releasePage(page_id, true);
        }

        pager.flushAll();
        std::cout << "数据已写入，文件大小: "
                  << std::filesystem::file_size(test_db + ".minidb") << " 字节" << std::endl;
    }

    // 第二阶段：重启后验证所有数据
    {
        std::cout << "\n重启后验证所有数据..." << std::endl;

        auto file_manager = std::make_shared<minidb::storage::FileManager>();
        file_manager->openDatabase(test_db);
        auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
        auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 50);
        minidb::storage::Pager pager(disk_manager, buffer_manager);

        std::cout << "总页面数: " << pager.getPageCount() << std::endl;

        // 验证每个页面
        for (minidb::PageID page_id = 1; page_id < 6; page_id++) {
            if (!pager.isPageInUse(page_id)) continue;

            minidb::storage::Page* page = pager.getPage(page_id);
            const auto& header = page->getHeader();

            std::cout << "验证页面 " << page_id << " (槽位数: " << header.slot_count << ")" << std::endl;

            // 验证每条记录
            for (uint16_t slot_num = 0; slot_num < header.slot_count; slot_num++) {
                minidb::RID rid{page_id, slot_num};
                char buffer[200];
                uint16_t size;

                bool found = page->getRecord(rid, buffer, &size);
                REQUIRE(found);

                // 检查记录格式
                REQUIRE(std::string(buffer).find("Page ") != std::string::npos);
                REQUIRE(std::string(buffer).find("Record ") != std::string::npos);
                REQUIRE(std::string(buffer).find("Test Data") != std::string::npos);

                std::cout << "  记录 " << slot_num << ": 验证成功 ✓" << std::endl;
            }

            pager.releasePage(page_id, false);
        }

        std::cout << "所有页面验证完成！" << std::endl;
    }

    // 清理
    if (std::filesystem::exists(test_db + ".minidb")) {
        std::filesystem::remove(test_db + ".minidb");
    }
}