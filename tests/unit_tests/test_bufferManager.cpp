// #include <../tests/catch2/catch_amalgamated.hpp>
// #include <storage/DiskManager.h>
// #include <storage/FileManager.h>
// #include <storage/Page.h>
// #include <common/Exception.h>
// #include <memory>
// #include <cstring>
// #include <thread>
// #include <vector>
//
// TEST_CASE("DiskManager basic functionality", "[diskmanager][storage][unit]")
// {
//     // 创建测试用的文件管理器
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     std::string test_db = "test_diskmanager_db";
//
//     // 清理可能存在的旧测试文件
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
//
//     SECTION("DiskManager construction and initialization") {
//         file_manager->createDatabase(test_db);
//         minidb::storage::DiskManager disk_manager(file_manager);
//
//         REQUIRE(disk_manager.getPageCount() == 1); // 初始应该有1个页面（头页面）
//         REQUIRE(disk_manager.getFreeListHead() == minidb::INVALID_PAGE_ID); // 初始空闲链表为空
//     }
//
//     SECTION("Page allocation and deallocation") {
//         file_manager->createDatabase(test_db);
//         minidb::storage::DiskManager disk_manager(file_manager);
//
//         // 分配页面
//         minidb::PageID page1 = disk_manager.allocatePage();
//         minidb::PageID page2 = disk_manager.allocatePage();
//
//         REQUIRE(page1 == 1); // 第一个分配的页面应该是ID 1
//         REQUIRE(page2 == 2); // 第二个分配的页面应该是ID 2
//         REQUIRE(disk_manager.getPageCount() == 3); // 头页面 + 2个数据页面
//
//         // 释放页面
//         disk_manager.deallocatePage(page1);
//         REQUIRE(disk_manager.getFreeListHead() == page1); // 空闲链表头应该是释放的页面
//
//         // 重新分配应该从空闲链表获取
//         minidb::PageID page3 = disk_manager.allocatePage();
//         REQUIRE(page3 == page1); // 应该重用释放的页面
//         REQUIRE(disk_manager.getFreeListHead() == minidb::INVALID_PAGE_ID); // 空闲链表应该为空
//     }
//
//     // 清理测试文件
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
// }
//
// TEST_CASE("DiskManager page read/write operations", "[diskmanager][io][unit]")
// {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     std::string test_db = "test_diskmanager_rw_db";
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
//
//     SECTION("Basic page write and read") {
//         file_manager->createDatabase(test_db);
//         minidb::storage::DiskManager disk_manager(file_manager);
//
//         // 分配页面
//         minidb::PageID page_id = disk_manager.allocatePage();
//
//         // 准备测试数据
//         char write_data[minidb::PAGE_SIZE];
//         char read_data[minidb::PAGE_SIZE];
//         std::memset(write_data, 0xAB, minidb::PAGE_SIZE);
//         std::strcpy(write_data, "Test page data");
//
//         // 写入页面
//         disk_manager.writePage(page_id, write_data);
//
//         // 读取页面
//         disk_manager.readPage(page_id, read_data);
//
//         // 验证数据一致性
//         REQUIRE(std::memcmp(write_data, read_data, minidb::PAGE_SIZE) == 0);
//         REQUIRE(std::strcmp(read_data, "Test page data") == 0);
//     }
//
//     SECTION("Multiple page operations") {
//         file_manager->createDatabase(test_db);
//         minidb::storage::DiskManager disk_manager(file_manager);
//
//         const int num_pages = 5;
//         std::vector<minidb::PageID> page_ids;
//
//         // 分配多个页面
//         for (int i = 0; i < num_pages; ++i) {
//             page_ids.push_back(disk_manager.allocatePage());
//         }
//
//         // 为每个页面写入不同的数据
//         for (size_t i = 0; i < page_ids.size(); ++i) {
//             char data[minidb::PAGE_SIZE];
//             std::memset(data, 0, minidb::PAGE_SIZE);
//             std::sprintf(data, "Page %d data", static_cast<int>(i));
//             disk_manager.writePage(page_ids[i], data);
//         }
//
//         // 验证每个页面的数据
//         for (size_t i = 0; i < page_ids.size(); ++i) {
//             char read_data[minidb::PAGE_SIZE];
//             disk_manager.readPage(page_ids[i], read_data);
//
//             char expected_data[minidb::PAGE_SIZE];
//             std::sprintf(expected_data, "Page %d data", static_cast<int>(i));
//
//             REQUIRE(std::strcmp(read_data, expected_data) == 0);
//         }
//     }
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
// }
//
// TEST_CASE("DiskManager edge cases", "[diskmanager][edge][unit]")
// {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     std::string test_db = "test_diskmanager_edge_db";
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
//
//     SECTION("Invalid page ID operations") {
//         file_manager->createDatabase(test_db);
//         minidb::storage::DiskManager disk_manager(file_manager);
//
//         char data[minidb::PAGE_SIZE];
//
//         // 读取无效页面应该抛出异常
//         REQUIRE_THROWS_AS(disk_manager.readPage(999, data), minidb::DiskException);
//
//         // 写入无效页面应该抛出异常
//         REQUIRE_THROWS_AS(disk_manager.writePage(999, data), minidb::DiskException);
//
//         // 释放无效页面应该抛出异常
//         REQUIRE_THROWS_AS(disk_manager.deallocatePage(999), minidb::DiskException);
//         REQUIRE_THROWS_AS(disk_manager.deallocatePage(0), minidb::DiskException); // 头页面不能释放
//     }
//
//     SECTION("Free list management") {
//         file_manager->createDatabase(test_db);
//         minidb::storage::DiskManager disk_manager(file_manager);
//
//         // 分配多个页面
//         minidb::PageID page1 = disk_manager.allocatePage();
//         minidb::PageID page2 = disk_manager.allocatePage();
//         minidb::PageID page3 = disk_manager.allocatePage();
//
//         // 释放页面并验证空闲链表
//         disk_manager.deallocatePage(page2);
//         REQUIRE(disk_manager.getFreeListHead() == page2);
//
//         disk_manager.deallocatePage(page1);
//         REQUIRE(disk_manager.getFreeListHead() == page1);
//
//         // 重新分配应该按LIFO顺序
//         minidb::PageID new_page = disk_manager.allocatePage();
//         REQUIRE(new_page == page1);
//         REQUIRE(disk_manager.getFreeListHead() == page2);
//
//         new_page = disk_manager.allocatePage();
//         REQUIRE(new_page == page2);
//         REQUIRE(disk_manager.getFreeListHead() == minidb::INVALID_PAGE_ID);
//     }
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
// }
//
// TEST_CASE("DiskManager persistence", "[diskmanager][persistence][unit]")
// {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     std::string test_db = "test_diskmanager_persist_db";
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
//
//     SECTION("Data persistence across instances") {
//         // 第一次创建和写入
//         {
//             file_manager->createDatabase(test_db);
//             minidb::storage::DiskManager disk_manager(file_manager);
//
//             minidb::PageID page_id = disk_manager.allocatePage();
//             char data[minidb::PAGE_SIZE];
//             std::strcpy(data, "Persistent data");
//             disk_manager.writePage(page_id, data);
//         }
//
//         // 重新打开验证数据持久化
//         {
//             file_manager->openDatabase(test_db);
//             minidb::storage::DiskManager disk_manager(file_manager);
//
//             REQUIRE(disk_manager.getPageCount() == 2); // 头页面 + 数据页面
//
//             char read_data[minidb::PAGE_SIZE];
//             disk_manager.readPage(1, read_data); // 页面ID 1
//
//             REQUIRE(std::strcmp(read_data, "Persistent data") == 0);
//         }
//     }
//
//     SECTION("Free list persistence") {
//         // 第一次创建、分配和释放
//         {
//             file_manager->createDatabase(test_db);
//             minidb::storage::DiskManager disk_manager(file_manager);
//
//             minidb::PageID page1 = disk_manager.allocatePage();
//             minidb::PageID page2 = disk_manager.allocatePage();
//             disk_manager.deallocatePage(page1);
//         }
//
//         // 重新打开验证空闲链表持久化
//         {
//             file_manager->openDatabase(test_db);
//             minidb::storage::DiskManager disk_manager(file_manager);
//
//             REQUIRE(disk_manager.getFreeListHead() == 1); // 页面1应该在空闲链表中
//
//             // 分配应该从空闲链表获取
//             minidb::PageID new_page = disk_manager.allocatePage();
//             REQUIRE(new_page == 1);
//             REQUIRE(disk_manager.getFreeListHead() == minidb::INVALID_PAGE_ID);
//         }
//     }
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
// }
//
// TEST_CASE("DiskManager integration with Page class", "[diskmanager][integration][unit]")
// {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     std::string test_db = "test_diskmanager_integration_db";
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
//
//     SECTION("Page serialization and deserialization") {
//         file_manager->createDatabase(test_db);
//         minidb::storage::DiskManager disk_manager(file_manager);
//
//         // 创建页面并插入数据
//         minidb::storage::Page page;
//         page.getHeader().page_id = disk_manager.allocatePage();
//
//         const char* test_data = "Test record data";
//         minidb::RID rid;
//         bool inserted = page.insertRecord(test_data, std::strlen(test_data) + 1, &rid);
//         REQUIRE(inserted);
//
//         // 序列化页面到磁盘
//         char page_buffer[minidb::PAGE_SIZE];
//         page.serialize(page_buffer);
//         disk_manager.writePage(page.getPageId(), page_buffer);
//
//         // 从磁盘读取并反序列化
//         char read_buffer[minidb::PAGE_SIZE];
//         disk_manager.readPage(page.getPageId(), read_buffer);
//
//         minidb::storage::Page restored_page;
//         restored_page.deserialize(read_buffer);
//
//         // 验证数据一致性
//         REQUIRE(restored_page.getPageId() == page.getPageId());
//         REQUIRE(restored_page.getHeader().slot_count == page.getHeader().slot_count);
//
//         char record_data[100];
//         uint16_t record_size;
//         bool found = restored_page.getRecord(rid, record_data, &record_size);
//         REQUIRE(found);
//         REQUIRE(std::strcmp(record_data, test_data) == 0);
//     }
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
// }
//
// TEST_CASE("DiskManager concurrent access", "[diskmanager][concurrency][unit]")
// {
//     auto file_manager = std::make_shared<minidb::storage::FileManager>();
//     std::string test_db = "test_diskmanager_concurrent_db";
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
//
//     SECTION("Thread-safe page operations") {
//         file_manager->createDatabase(test_db);
//         auto disk_manager = std::make_shared<minidb::storage::DiskManager>(file_manager);
//
//         const int num_threads = 4;
//         const int pages_per_thread = 10;
//         std::vector<std::thread> threads;
//         std::atomic<int> success_count{0};
//
//         for (int i = 0; i < num_threads; ++i) {
//             threads.emplace_back([&, i]() {
//                 try {
//                     for (int j = 0; j < pages_per_thread; ++j) {
//                         minidb::PageID page_id = disk_manager->allocatePage();
//
//                         char data[minidb::PAGE_SIZE];
//                         std::memset(data, i + 'A', minidb::PAGE_SIZE);
//                         std::sprintf(data, "Thread %d, Page %d", i, j);
//
//                         disk_manager->writePage(page_id, data);
//
//                         char read_data[minidb::PAGE_SIZE];
//                         disk_manager->readPage(page_id, read_data);
//
//                         if (std::strcmp(data, read_data) == 0) {
//                             success_count++;
//                         }
//                     }
//                 } catch (const std::exception& e) {
//                     // 线程中的异常会被捕获，不会影响其他线程
//                 }
//             });
//         }
//
//         for (auto& thread : threads) {
//             thread.join();
//         }
//
//         // 验证所有操作都成功
//         REQUIRE(success_count == num_threads * pages_per_thread);
//         REQUIRE(disk_manager->getPageCount() == 1 + num_threads * pages_per_thread);
//     }
//
//     if (file_manager->databaseExists(test_db)) {
//         file_manager->deleteDatabase(test_db);
//     }
// }