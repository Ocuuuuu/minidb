#include "../include/storage/FileManager.h"
#include "common/Exception.h"
#include <filesystem>
#include <iostream>
#include <regex>
#include <algorithm>
#include <vector>

namespace minidb {
namespace storage {

FileManager::FileManager() : is_open_(false) {}

FileManager::~FileManager() {
    if (is_open_) {
        closeDatabase();
    }
}

    bool FileManager::isValidDatabaseName(const std::string& name) {
    if (name.empty()) {
        return false;
    }

    // 检查非法字符
#ifdef _WIN32
    // Windows 非法字符: < > : " | ? * \
    // 将正则表达式移到静态方法内部
    const std::regex win_invalid_chars(R"([<>:"|?*\\])");  // 移除static
    try {
        if (std::regex_search(name, win_invalid_chars)) {
            return false;
        }
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << std::endl;
        return false;
    }
#else
    // Unix 非法字符: / 和 null 字符
    if (name.find('/') != std::string::npos ||
        name.find('\0') != std::string::npos) {
        return false;
        }
#endif

    // 检查保留名称（Windows）
#ifdef _WIN32
    static const std::vector<std::string> reserved_names = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    std::string upper_name = name;
    std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);

    for (const auto& reserved : reserved_names) {
        if (upper_name == reserved) {
            return false;
        }
    }
#endif

    // 检查文件名长度限制
    if (name.length() > 255) {
        return false;
    }

    // 检查是否以点开头（隐藏文件）或包含连续的点
    if (name[0] == '.' || name.find("..") != std::string::npos) {
        return false;
    }

    return true;
}

void FileManager::createDatabase(const std::string& db_name) {
    if (is_open_) {
        closeDatabase();
    }

    // 验证数据库名
    if (db_name.empty()) {
        throw IOException("Database name cannot be empty");
    }

    if (!isValidDatabaseName(db_name)) {
        throw IOException("Invalid database name: " + db_name);
    }

    db_name_ = db_name;
    db_path_ = db_name_ + ".minidb";

    // 确保目录存在
    std::filesystem::path db_path_obj(db_path_);
    if (db_path_obj.has_parent_path()) {
        ensureDirectoryExists(db_path_obj.parent_path().string());
    }

    // // 检查文件是否已存在
    // if (std::filesystem::exists(db_path_)) {
    //     throw IOException("Database file already exists: " + db_path_);
    // }

    // 检查文件是否已存在并删除
    if (std::filesystem::exists(db_path_)) {
        std::cout << "Database file already exists. Deleting it..." << std::endl;
        std::filesystem::remove(db_path_);
    }

    // 创建文件
    db_file_.open(db_path_, std::ios::out | std::ios::binary);
    if (!db_file_.is_open()) {
        throw IOException("Cannot create database file: " + db_path_);
    }
    db_file_.close();

    // 重新以读写模式打开
    openDatabase(db_name);
}

    void FileManager::openDatabase(const std::string& db_name) {
    if (is_open_) {
        closeDatabase();
    }

    // 验证数据库名
    if (db_name.empty()) {
        throw DatabaseNotOpenException("Database name cannot be empty");
    }

    if (!isValidDatabaseName(db_name)) {
        throw DatabaseNotOpenException("Invalid database name: " + db_name);
    }

    db_name_ = db_name;
    db_path_ = db_name_ + ".minidb";

    // 确保目录存在
    std::filesystem::path db_path_obj(db_path_);
    if (db_path_obj.has_parent_path()) {
        ensureDirectoryExists(db_path_obj.parent_path().string());
    }

    // 使用正确的打开模式：如果文件不存在则创建
    db_file_.open(db_path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);

    if (!db_file_.is_open()) {
        // 如果文件不存在，尝试以 out 模式创建
        db_file_.open(db_path_, std::ios::out | std::ios::binary);
        if (db_file_.is_open()) {
            db_file_.close();
            // 重新以正确的模式打开
            db_file_.open(db_path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
        }

        if (!db_file_.is_open()) {
            throw DatabaseNotOpenException("Cannot open database file: " + db_path_);
        }
    }

    // 确保文件指针在开头
    db_file_.seekg(0, std::ios::beg);
    db_file_.seekp(0, std::ios::beg);

    is_open_ = true;
}

void FileManager::closeDatabase() {
    if (db_file_.is_open()) {
        db_file_.close();
    }
    is_open_ = false;
    db_name_.clear();
    db_path_.clear();
}

bool FileManager::isOpen() const {
    return is_open_;
}

std::fstream& FileManager::getFileStream() {
    if (!is_open_) {
        throw DatabaseNotOpenException("Database is not open");
    }
    return db_file_;
}

const std::string& FileManager::getDatabasePath() const {
    return db_path_;
}

const std::string& FileManager::getDatabaseName() const {
    return db_name_;
}

void FileManager::ensureDirectoryExists(const std::string& path) {
    if (path.empty()) return;

    try {
        std::filesystem::create_directories(path);
    } catch (const std::filesystem::filesystem_error& e) {
        throw IOException("Cannot create directory: " + path + " - " + e.what());
    } catch (const std::exception& e) {
        throw IOException("Cannot create directory: " + path + " - " + e.what());
    }
}

bool FileManager::databaseExists(const std::string& db_name) {
    if (!isValidDatabaseName(db_name)) {
        return false;
    }

    std::string db_path = db_name + ".minidb";
    return std::filesystem::exists(db_path);
}

void FileManager::deleteDatabase(const std::string& db_name) {
    if (is_open_ && db_name_ == db_name) {
        closeDatabase();
    }

    if (!isValidDatabaseName(db_name)) {
        throw IOException("Invalid database name: " + db_name);
    }

    std::string db_path = db_name + ".minidb";

    if (!std::filesystem::exists(db_path)) {
        throw IOException("Database file does not exist: " + db_path);
    }

    try {
        std::filesystem::remove(db_path);
    } catch (const std::filesystem::filesystem_error& e) {
        throw IOException("Cannot delete database file: " + db_path + " - " + e.what());
    }
}

} // namespace storage
} // namespace minidb