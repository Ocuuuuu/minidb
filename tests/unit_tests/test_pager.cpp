// #include <../tests/catch2/catch_amalgamated.hpp>
// #include "storage/Pager.h"
// #include "storage/DiskManager.h"
// #include "storage/BufferManager.h"
// #include "storage/FileManager.h"
// #include "common/Exception.h"
// #include <memory>
// #include <thread>
//
// TEST_CASE("Pager Construction and Basic Properties", "[storage][pager]") {
//     SECTION("Pager can be constructed with disk and buffer managers") {
//         auto file_manager = std::make_shared<minidb::storage::FileManager>();
//         file_manager->openDatabase("test_db");
//         auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
//         auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 10);
//
//         minidb::storage::Pager pager(disk_manager, buffer_manager);
//
//         REQUIRE(pager.getPageCount() >= 0);
//     }
// }
//
// TEST_CASE("Pager Page Allocation and Deallocation", "[storage][pager]") {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     file_manager->openDatabase("test_db_alloc");
//     auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
//     auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 10);
//     minidb::storage::Pager pager(disk_manager, buffer_manager);
//
//     SECTION("Allocate page returns valid page ID") {
//         minidb::PageID page_id = pager.allocatePage();
//
//         REQUIRE(page_id != minidb::INVALID_PAGE_ID);
//         REQUIRE(pager.isPageInUse(page_id));
//     }
//
//     SECTION("Allocate multiple pages returns unique IDs") {
//         minidb::PageID page1 = pager.allocatePage();
//         minidb::PageID page2 = pager.allocatePage();
//         minidb::PageID page3 = pager.allocatePage();
//
//         REQUIRE(page1 != page2);
//         REQUIRE(page2 != page3);
//         REQUIRE(page1 != page3);
//         REQUIRE(pager.isPageInUse(page1));
//         REQUIRE(pager.isPageInUse(page2));
//         REQUIRE(pager.isPageInUse(page3));
//     }
//
//     SECTION("Deallocate page removes it from use") {
//         minidb::PageID page_id = pager.allocatePage();
//         REQUIRE(pager.isPageInUse(page_id));
//
//         pager.deallocatePage(page_id);
//         REQUIRE_FALSE(pager.isPageInUse(page_id));
//     }
//
//     SECTION("Deallocate invalid page ID throws no exception") {
//         // Should not throw for invalid page ID
//         pager.deallocatePage(minidb::INVALID_PAGE_ID);
//         pager.deallocatePage(9999); // Non-existent page
//     }
// }
//
// TEST_CASE("Pager Page Access Operations", "[storage][pager]") {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     file_manager->openDatabase("test_db_access");
//     auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
//     auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 10);
//     minidb::storage::Pager pager(disk_manager, buffer_manager);
//
//     SECTION("Get page returns valid page pointer") {
//         minidb::PageID page_id = pager.allocatePage();
//
//         // 分配页面后应该自动被pin住，所以可以直接获取
//         minidb::storage::Page* page = pager.getPage(page_id);
//         REQUIRE(page != nullptr);
//         REQUIRE(page->getPageId() == page_id);
//
//         // 释放页面
//         pager.releasePage(page_id, false);
//
//         // 清理
//         pager.deallocatePage(page_id);
//     }
//
//     SECTION("Get non-existent page throws exception") {
//         // 使用一个明显超出范围的页面ID
//         REQUIRE_THROWS_AS(pager.getPage(999999), std::runtime_error);
//         REQUIRE_THROWS(pager.getPage(999999));
//     }
//
//     SECTION("Pin and release page operations work") {
//         minidb::PageID page_id = pager.allocatePage();
//
//         // 分配后页面应该已经被pin，可以获取
//         minidb::storage::Page* page = pager.getPage(page_id);
//         REQUIRE(page != nullptr);
//
//         // 再次pin页面（增加pin计数）
//         pager.pinPage(page_id);
//
//         // 释放页面（减少pin计数）
//         pager.releasePage(page_id, false);
//
//         // 再次释放（应该完全unpin）
//         pager.releasePage(page_id, false);
//
//         // 清理：释放分配的页面
//         pager.deallocatePage(page_id);
//     }
//
//     SECTION("Test page pinning and dirty flag") {
//         minidb::PageID page_id = pager.allocatePage();
//
//         // 获取页面
//         minidb::storage::Page* page = pager.getPage(page_id);
//         REQUIRE(page != nullptr);
//
//         // 修改页面内容
//         char* data = page->getData();
//         data[0] = 'X'; // 简单修改
//
//         // 释放页面并标记为dirty
//         pager.releasePage(page_id, true);
//
//         // 清理
//         pager.deallocatePage(page_id);
//     }
//
//     // 添加清理代码
//     file_manager->closeDatabase();
// }
//
// TEST_CASE("Pager Flush Operations", "[storage][pager]") {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     file_manager->openDatabase("test_db_flush");
//     auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
//     auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 10);
//     minidb::storage::Pager pager(disk_manager, buffer_manager);
//
//     SECTION("Flush single page works") {
//         minidb::PageID page_id = pager.allocatePage();
//
//         // 确保页面在缓冲区中
//         auto page = pager.getPage(page_id);
//         REQUIRE(page != nullptr);
//
//         // 修改页面内容使其成为脏页
//         char* data = page->getData();
//         data[0] = 'X';
//
//         // 释放页面并标记为脏
//         pager.releasePage(page_id, true);
//
//         // 刷新页面 - 应该不会抛出异常
//         REQUIRE_NOTHROW(pager.flushPage(page_id));
//
//         // 清理
//         pager.deallocatePage(page_id);
//     }
//
//     SECTION("Flush all pages works") {
//         minidb::PageID page1 = pager.allocatePage();
//         minidb::PageID page2 = pager.allocatePage();
//         minidb::PageID page3 = pager.allocatePage();
//
//         // 获取并修改页面使其成为脏页
//         auto page_ptr1 = pager.getPage(page1);
//         auto page_ptr2 = pager.getPage(page2);
//         auto page_ptr3 = pager.getPage(page3);
//
//         page_ptr1->getData()[0] = 'A';
//         page_ptr2->getData()[0] = 'B';
//         page_ptr3->getData()[0] = 'C';
//
//         // 释放页面并标记为脏
//         pager.releasePage(page1, true);
//         pager.releasePage(page2, true);
//         pager.releasePage(page3, true);
//
//         // 刷新所有页面 - 应该不会抛出异常
//         REQUIRE_NOTHROW(pager.flushAll());
//
//         // 清理
//         pager.deallocatePage(page1);
//         pager.deallocatePage(page2);
//         pager.deallocatePage(page3);
//     }
//
//     SECTION("Flush non-existent page does not throw") {
//         // 刷新不存在的页面应该不会抛出异常（或者应该优雅处理）
//         // 这取决于你的实现，可能需要检查flushPage对无效ID的处理
//
//         // 如果flushPage对无效ID抛出异常，使用REQUIRE_THROWS
//         // 如果不抛出异常，使用REQUIRE_NOTHROW
//
//         // 先检查你的实现方式
//         try {
//             pager.flushPage(9999);
//             // 如果不抛出异常，测试通过
//             SUCCEED("flushPage with invalid ID handled gracefully");
//         } catch (const std::exception& e) {
//             // 如果抛出异常，确保是预期的异常类型
//             REQUIRE_THROWS_AS(pager.flushPage(9999), std::runtime_error);
//         }
//
//         // 对INVALID_PAGE_ID做同样处理
//         try {
//             pager.flushPage(minidb::INVALID_PAGE_ID);
//             SUCCEED("flushPage with INVALID_PAGE_ID handled gracefully");
//         } catch (const std::exception& e) {
//             REQUIRE_THROWS_AS(pager.flushPage(minidb::INVALID_PAGE_ID), std::runtime_error);
//         }
//     }
//
//     // 添加清理代码
//     file_manager->closeDatabase();
// }
//
// TEST_CASE("Pager Thread Safety", "[storage][pager][concurrency]") {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     file_manager->openDatabase("test_db_thread");
//     auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
//     auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 20);
//     minidb::storage::Pager pager(disk_manager, buffer_manager);
//
//     SECTION("Concurrent page allocation") {
//         const int num_threads = 4;
//         const int pages_per_thread = 5;
//         std::vector<std::thread> threads;
//         std::vector<minidb::PageID> allocated_pages;
//         std::mutex pages_mutex;
//
//         for (int i = 0; i < num_threads; i++) {
//             threads.emplace_back([&]() {
//                 for (int j = 0; j < pages_per_thread; j++) {
//                     minidb::PageID page_id = pager.allocatePage();
//                     std::lock_guard<std::mutex> lock(pages_mutex);
//                     allocated_pages.push_back(page_id);
//                 }
//             });
//         }
//
//         for (auto& thread : threads) {
//             thread.join();
//         }
//
//         REQUIRE(allocated_pages.size() == num_threads * pages_per_thread);
//
//         // Check that all pages are unique and in use
//         std::set<minidb::PageID> unique_pages;
//         for (minidb::PageID page_id : allocated_pages) {
//             REQUIRE(pager.isPageInUse(page_id));
//             unique_pages.insert(page_id);
//         }
//         REQUIRE(unique_pages.size() == allocated_pages.size());
//     }
// }
//
// TEST_CASE("Pager Page Usage Tracking", "[storage][pager]") {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     file_manager->openDatabase("test_db_tracking");
//     auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
//     auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 10);
//     minidb::storage::Pager pager(disk_manager, buffer_manager);
//
//     SECTION("isPageInUse returns correct status") {
//         minidb::PageID page_id = pager.allocatePage();
//         REQUIRE(pager.isPageInUse(page_id));
//
//         pager.deallocatePage(page_id);
//         REQUIRE_FALSE(pager.isPageInUse(page_id));
//     }
//
//     SECTION("isPageInUse for invalid page IDs") {
//         REQUIRE_FALSE(pager.isPageInUse(minidb::INVALID_PAGE_ID));
//         REQUIRE_FALSE(pager.isPageInUse(9999)); // Non-existent page
//     }
// }
//
// TEST_CASE("Pager Integration with BufferManager", "[storage][pager][integration]") {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     file_manager->openDatabase("test_db_integration");
//     auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
//     auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 5);
//     minidb::storage::Pager pager(disk_manager, buffer_manager);
//
//     SECTION("Page fetched through pager is properly managed by buffer") {
//         minidb::PageID page_id = pager.allocatePage();
//
//         // Get page through pager
//         minidb::storage::Page* page = pager.getPage(page_id);
//         REQUIRE(page != nullptr);
//
//         // Modify the page
//         page->setDirty(true);
//
//         // Release through pager
//         pager.releasePage(page_id, true);
//
//         // Flush through pager
//         pager.flushPage(page_id);
//     }
//
//     SECTION("Buffer manager statistics are accessible through pager") {
//         // Allocate and access some pages to generate statistics
//         minidb::PageID page1 = pager.allocatePage();
//         minidb::PageID page2 = pager.allocatePage();
//
//         minidb::storage::Page* page_ptr1 = pager.getPage(page1);
//         minidb::storage::Page* page_ptr2 = pager.getPage(page2);
//
//         // Access the pages multiple times
//         for (int i = 0; i < 3; i++) {
//             pager.getPage(page1);
//         }
//
//         // Statistics should reflect the accesses
//         // (Note: This depends on BufferManager implementation)
//     }
// }
//
// // TEST_CASE("Pager Checkpoint Functionality", "[storage][pager]") {
// //     auto file_manager = std::make_shared<minidb::storage::FileManager>();
// //     file_manager->openDatabase("test_db_checkpoint");
// //     auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
// //     auto buffer_manager = std::make_shared<minidb::storage::BufferManager>(disk_manager, 10);
// //     minidb::storage::Pager pager(disk_manager, buffer_manager);
// //
// //     SECTION("Checkpoint method exists and can be called") {
// //         // Should not throw (even if not implemented)
// //         pager.checkpoint();
// //     }
// // }