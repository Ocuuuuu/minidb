#include "../../include/common/Exception.h"
#include "../../include/storage/FileManager.h"
#include "../../include/common/Constants.h"
#include <algorithm>
#include <bemapiset.h>
#include <iostream>
#include <thread>
#include <windows.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#endif

namespace minidb {
namespace storage {

FileManager::FileManager() : is_open_(false) {}

FileManager::~FileManager() {
    if (is_open_) {
        closeDatabase();
    }
}

    void FileManager::createDatabase(const std::string& db_name) {
    if (is_open_) {
        closeDatabase();
    }

    db_name_ = db_name;
    db_path_ = db_name_ + ".minidb";

    // 确保目录存在
    size_t pos = db_path_.find_last_of("/\\");
    if (pos != std::string::npos) {
        ensureDirectoryExists(db_path_.substr(0, pos));
    }

    // 二进制模式创建文件并写入默认头
    std::ofstream create_stream(db_path_, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!create_stream.is_open()) {
        throw FileNotFoundException("Failed to create database: " + db_path_);
    }

    // 写入默认头信息（PageCount=1，FreeListHead=-1）
    PageID default_page_count = 1;
    PageID default_free_list_head = INVALID_PAGE_ID;  // -1

    // 调试：打印写入的二进制值（16进制）
    std::cout << "FileManager: Writing PageCount (hex): 0x" << std::hex << default_page_count << std::dec << std::endl;
    std::cout << "FileManager: Writing FreeListHead (hex): 0x" << std::hex << default_free_list_head << std::dec << std::endl;

    // 写入页计数（4字节）
    create_stream.write(reinterpret_cast<const char*>(&default_page_count), sizeof(PageID));
    // 写入空闲链表头（4字节）
    create_stream.write(reinterpret_cast<const char*>(&default_free_list_head), sizeof(PageID));

    // 确保文件大小为8字节
    create_stream.seekp(FILE_HEADER_SIZE - 1);
    create_stream.put('\0');
    create_stream.flush();

    if (create_stream.fail()) {
        throw FileException("Failed to write default header");
    }
    create_stream.close();  // 关闭流，强制数据写入磁盘

    std::cout << "FileManager: Created database with valid header - " << db_path_ << std::endl;

    // 以二进制模式打开
    openDatabase(db_name);
}

    // FileManager::openDatabase 修复
    void FileManager::openDatabase(const std::string& db_name) {
    if (is_open_) {
        closeDatabase();
    }

    db_name_ = db_name;
    db_path_ = db_name_ + ".minidb";

    // 检查文件是否存在
    std::ifstream test_file(db_path_, std::ios::binary);  // 二进制模式检查
    if (!test_file.good()) {
        throw FileNotFoundException(db_path_);
    }
    test_file.close();

    // 强制二进制模式打开，避免文本模式的自动转换
    db_file_.open(db_path_, std::ios::in | std::ios::out | std::ios::binary);
    if (!db_file_.is_open()) {
        throw FileException("Failed to open database: " + db_path_);
    }

    is_open_ = true;
    std::cout << "FileManager: Opened database in binary mode - " << db_path_ << std::endl;
}



void FileManager::closeDatabase() {
    if (db_file_.is_open()) {
        db_file_.close();
        db_file_.clear();
    }
    is_open_ = false;
    // 给系统100毫秒释放文件（跨平台通用）
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}


bool FileManager::isOpen() const {
    return is_open_;
}

std::fstream& FileManager::getFileStream() {
    if (!is_open_) {
        throw DatabaseNotOpenException();
    }
    return db_file_;
}

const std::string& FileManager::getDatabasePath() const {
    return db_path_;
}

    void FileManager::ensureDirectoryExists(const std::string& path) {
    if (path.empty()) return;

#ifdef _WIN32
    // 强制将 '/' 转换为 Windows 路径分隔符 '\'
    std::string adjusted_path = path;
    std::replace(adjusted_path.begin(), adjusted_path.end(), '/', '\\');

    size_t pos = 0;
    // 跳过盘符（如 "C:\\"）
    if (adjusted_path.size() >= 2 && adjusted_path[1] == ':') {
        pos = 2;
        if (adjusted_path[pos] == '\\') pos++;
    }

    while (pos < adjusted_path.size()) {
        pos = adjusted_path.find('\\', pos);
        std::string subdir = adjusted_path.substr(0, pos);
        if (subdir.empty()) {
            pos++;
            continue;
        }

        // 创建目录并严格校验返回值
        if (_mkdir(subdir.c_str()) != 0) {
            // 只有 "目录已存在" 才忽略，其他错误均抛出异常
            if (errno != EEXIST) {
                throw DirectoryCreationException("Failed to create directory: " + subdir);
            }
        }
        pos++;
    }
#else
    // Linux/Unix 逻辑不变
    mode_t mode = 0777;
    size_t pos = 0;
    while (pos < path.size()) {
        pos = path.find('/', pos + 1);
        std::string subdir = path.substr(0, pos);
        if (subdir.empty()) continue;

        if (mkdir(subdir.c_str(), mode) != 0 && errno != EEXIST) {
            throw DirectoryCreationException("Failed to create directory: " + subdir);
        }
    }
#endif
}

} // namespace storage
} // namespace minidb