// #include <../tests/catch2/catch_amalgamated.hpp>
// #include <storage/FileManager.h>
// #include <common/Exception.h>
// #include <filesystem>
// #include <fstream>
//
// namespace fs = std::filesystem;
//
// TEST_CASE("FileManager basic functionality", "[filemanager][storage][unit]")
// {
//     // 创建临时测试目录和文件路径
//     std::string test_dir = "test_filemanager";
//     std::string test_db = test_dir + "/test_db";
//
//     // 确保测试开始时环境干净
//     if (fs::exists(test_db + ".minidb")) {
//         fs::remove(test_db + ".minidb");
//     }
//     if (fs::exists(test_dir)) {
//         fs::remove_all(test_dir);
//     }
//
//     minidb::storage::FileManager file_manager;
//
//     SECTION("Database creation and opening") {
//         REQUIRE(file_manager.isOpen() == false);
//
//         // 测试创建数据库
//         REQUIRE_NOTHROW(file_manager.createDatabase(test_db));
//         REQUIRE(file_manager.isOpen() == true);
//         REQUIRE(fs::exists(test_db + ".minidb") == true);
//
//         // 测试关闭数据库
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//         REQUIRE(file_manager.isOpen() == false);
//
//         // 测试重新打开数据库
//         REQUIRE_NOTHROW(file_manager.openDatabase(test_db));
//         REQUIRE(file_manager.isOpen() == true);
//
//         // 测试获取文件流
//         REQUIRE_NOTHROW(file_manager.getFileStream());
//         auto& stream = file_manager.getFileStream();
//         REQUIRE(stream.is_open() == true);
//
//         // 测试获取数据库路径
//         REQUIRE(file_manager.getDatabasePath() == test_db + ".minidb");
//
//         // 清理
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//     }
//
//     SECTION("Open non-existent database with auto-creation") {
//         std::string auto_create_db = test_dir + "/auto_create_db";
//
//         if (fs::exists(auto_create_db + ".minidb")) {
//             fs::remove(auto_create_db + ".minidb");
//         }
//
//         REQUIRE_NOTHROW(file_manager.openDatabase(auto_create_db));
//         REQUIRE(file_manager.isOpen() == true);
//         REQUIRE(fs::exists(auto_create_db + ".minidb") == true);
//
//         // 清理
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//         fs::remove(auto_create_db + ".minidb");
//     }
//
//     SECTION("Nested directory creation") {
//         std::string nested_db = test_dir + "/subdir1/subdir2/nested_db";
//
//         // 确保目录不存在
//         if (fs::exists(nested_db + ".minidb")) {
//             fs::remove(nested_db + ".minidb");
//         }
//         if (fs::exists(test_dir + "/subdir1")) {
//             fs::remove_all(test_dir + "/subdir1");
//         }
//
//         // 测试创建嵌套目录的数据库
//         REQUIRE_NOTHROW(file_manager.createDatabase(nested_db));
//         REQUIRE(file_manager.isOpen() == true);
//         REQUIRE(fs::exists(nested_db + ".minidb") == true);
//
//         // 清理
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//     }
//
//     // 清理测试目录
//     if (fs::exists(test_dir)) {
//         fs::remove_all(test_dir);
//     }
// }
//
// TEST_CASE("FileManager error handling", "[filemanager][error][unit]")
// {
//     minidb::storage::FileManager file_manager;
//
//     SECTION("Get file stream when database is closed") {
//         REQUIRE_THROWS_AS(file_manager.getFileStream(), minidb::DatabaseNotOpenException);
//     }
//
//     SECTION("Operations on closed database") {
//         std::string test_db = "test_closed_ops";
//
//         // 先创建再关闭
//         REQUIRE_NOTHROW(file_manager.createDatabase(test_db));
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//
//         // 关闭后操作应该抛出异常
//         REQUIRE_THROWS_AS(file_manager.getFileStream(), minidb::DatabaseNotOpenException);
//
//         // 清理
//         if (fs::exists(test_db + ".minidb")) {
//             fs::remove(test_db + ".minidb");
//         }
//     }
//
//     SECTION("Multiple close operations") {
//         std::string test_db = "test_multiple_close";
//
//         REQUIRE_NOTHROW(file_manager.createDatabase(test_db));
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//         // 多次关闭应该是幂等的，不应该抛出异常
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//
//         // 清理
//         if (fs::exists(test_db + ".minidb")) {
//             fs::remove(test_db + ".minidb");
//         }
//     }
// }
//
// TEST_CASE("FileManager file operations", "[filemanager][fileops][unit]")
// {
//     std::string test_dir = "test_fileops";
//     std::string test_db = test_dir + "/file_ops_db";
//
//     // 清理旧文件
//     if (fs::exists(test_db + ".minidb")) {
//         fs::remove(test_db + ".minidb");
//     }
//     if (fs::exists(test_dir)) {
//         fs::remove_all(test_dir);
//     }
//
//     minidb::storage::FileManager file_manager;
//
//     SECTION("File read/write operations through stream") {
//         REQUIRE_NOTHROW(file_manager.createDatabase(test_db));
//
//         auto& stream = file_manager.getFileStream();
//
//         // 测试写入数据
//         std::string test_data = "Hello, MiniDB!";
//         stream.write(test_data.c_str(), test_data.size());
//         REQUIRE(stream.good() == true);
//
//         // 测试读取数据
//         stream.seekg(0);
//         char buffer[100];
//         stream.read(buffer, test_data.size());
//         buffer[test_data.size()] = '\0';
//
//         REQUIRE(std::string(buffer) == test_data);
//
//         // 清理
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//     }
//
//     SECTION("File positioning") {
//         REQUIRE_NOTHROW(file_manager.createDatabase(test_db));
//
//         auto& stream = file_manager.getFileStream();
//
//         // 测试文件定位
//         stream.seekp(100);
//         REQUIRE(stream.tellp() == 100);
//
//         stream.seekg(50);
//         REQUIRE(stream.tellg() == 50);
//
//         // 清理
//         REQUIRE_NOTHROW(file_manager.closeDatabase());
//     }
//
//     // 清理测试目录
//     if (fs::exists(test_dir)) {
//         fs::remove_all(test_dir);
//     }
// }
//
// TEST_CASE("FileManager edge cases", "[filemanager][edge][unit]")
// {
//     minidb::storage::FileManager file_manager;
//
//     SECTION("Empty database name") {
//         REQUIRE_THROWS_AS(file_manager.createDatabase(""), minidb::IOException);
//         REQUIRE_THROWS_AS(file_manager.openDatabase(""), minidb::DatabaseNotOpenException);
//     }
//
//     SECTION("Invalid characters in database name") {
//         // 测试包含非法字符的数据库名
//         #ifdef _WIN32
//         // Windows 文件名中不能包含这些字符
//         std::string invalid_name = "test<>.db";
//         #else
//         // Unix 文件名中不能包含斜杠
//         std::string invalid_name = "test/.db";
//         #endif
//
//         REQUIRE_THROWS_AS(file_manager.createDatabase(invalid_name), minidb::IOException);
//     }
//
//     SECTION("Very long database name") {
//         std::string long_name(1000, 'a');
//         REQUIRE_THROWS_AS(file_manager.createDatabase(long_name), minidb::IOException);
//     }
// }
//
// TEST_CASE("FileManager concurrent access simulation", "[filemanager][concurrent][unit]")
// {
//     std::string test_db = "concurrent_test_db";
//
//     // 清理旧文件
//     if (fs::exists(test_db + ".minidb")) {
//         fs::remove(test_db + ".minidb");
//     }
//
//     minidb::storage::FileManager file_manager1;
//     minidb::storage::FileManager file_manager2;
//
//     SECTION("Multiple managers same database") {
//         // 第一个管理器创建数据库
//         REQUIRE_NOTHROW(file_manager1.createDatabase(test_db));
//
//         // 第二个管理器尝试打开同一个数据库
//         // 在 Windows 上，std::fstream 可能允许共享读取
//         try {
//             file_manager2.openDatabase(test_db);
//
//             // 如果成功打开，验证可以获取文件流
//             REQUIRE(file_manager2.isOpen());
//             REQUIRE_NOTHROW(file_manager2.getFileStream());
//
//             // 然后关闭第二个管理器
//             REQUIRE_NOTHROW(file_manager2.closeDatabase());
//             SUCCEED("Second open succeeded - file sharing is allowed");
//
//         } catch (const minidb::DatabaseNotOpenException& e) {
//             // 如果抛出异常，这也是可以接受的行为
//             SUCCEED("Second open failed as expected: " + std::string(e.what()));
//         } catch (const std::exception& e) {
//             FAIL("Unexpected exception: " + std::string(e.what()));
//         }
//
//         // 清理
//         REQUIRE_NOTHROW(file_manager1.closeDatabase());
//     }
//
//     // 清理
//     if (fs::exists(test_db + ".minidb")) {
//         fs::remove(test_db + ".minidb");
//     }
// }
//
// // 清理函数，确保测试后环境干净
// void cleanup_test_files() {
//     std::vector<std::string> test_dbs = {
//         "test_db", "auto_create_db", "test_closed_ops",
//         "test_multiple_close", "file_ops_db", "concurrent_test_db"
//     };
//
//     for (const auto& db : test_dbs) {
//         if (fs::exists(db + ".minidb")) {
//             fs::remove(db + ".minidb");
//         }
//     }
//
//     if (fs::exists("test_filemanager")) {
//         fs::remove_all("test_filemanager");
//     }
//     if (fs::exists("test_fileops")) {
//         fs::remove_all("test_fileops");
//     }
// }
//
// // 注册清理函数
// struct FileManagerTestCleanup {
//     ~FileManagerTestCleanup() {
//         cleanup_test_files();
//     }
// };
//
// FileManagerTestCleanup test_cleanup;