//
// Created by tang_ on 2025/9/9.
//

#include "../../include/storage/FileManager.h"

#include <cstring>
#include <filesystem>
#include <iostream>

#include "common/Types.h"

namespace minidb {
namespace storage {

namespace fs = std::filesystem;

     FileManager::FileManager() : db_name_(""), db_path_(""), is_open_(false) {}

     FileManager::~FileManager() {
    if (is_open_) {
        closeDatabase();
    }
}

void FileManager::createDatabase(const std::string& db_name) {

         if (is_open_) {
             closeDatabase();
         }
         //拼接文件路径
         db_name_ = db_name;
         db_path_ = db_name + DB_FILE_EXTENSION;

         // 确保目录存在
         //fs::path是std::fileesystem库中的路径类，用于表示文件系统路径，会自动处理不同操作系统的路径分隔符
         fs::path path_obj(db_path_);
         //parent_path()返回当前路径的父目录路径
         ensureDirectoryExists(path_obj.parent_path().string());

         try {
             // 创建数据库文件，std::ios::out一输出模式打开，这会创建新文件（如果不存在）
             //std::ios::linary以二进制模式打开，而非文本模式
             db_file_.open(db_path_, std::ios::out | std::ios::binary);
             if (!db_file_.is_open()) {
                 throw DatabaseException("Failed to create database file: " + db_path_);
             }
             db_file_.close();

             // 重新以读写模式打开
             openDatabase(db_name);

             // 初始化元数据，会在数据库文件的固定位置（文件开头）写入初始元数据，确保数据库文件创建之初就具备合法的结构信息
             initializeMetadata();

         } catch (const std::exception& e) {
             std::cerr << "Error creating database: " << e.what() << std::endl;
             throw;
         }
     }

void FileManager::openDatabase(const std::string& db_name) {
    if (is_open_) {
        closeDatabase();
    }

    db_name_ = db_name;
    db_path_ = db_name + DB_FILE_EXTENSION;

    // 检查文件是否存在
    if (!fs::exists(db_path_)) {
        throw DatabaseException("Database file does not exist: " + db_path_);
    }

    // 打开文件
    db_file_.open(db_path_, std::ios::in | std::ios::out | std::ios::binary);
    if (!db_file_.is_open()) {
        throw DatabaseException("Failed to open database file: " + db_path_);
    }

    is_open_ = true;

    // 加载元数据,将数据库文件中存储的元数据读取到内存中，够数据库引擎在运行时使用
    loadMetadata();
}

void FileManager::closeDatabase() {
    if (is_open_) {
        saveMetadata();
        db_file_.close();
        is_open_ = false;
    }
}

bool FileManager::isOpen() const {
    return is_open_;
}

// 提供数据库文件的流对象访问接口，允许数据库其他模块直接操作底层文件流
std::fstream& FileManager::getFileStream() {
         std::cout << "getFileStream called" << std::endl;
         if (!is_open_) {
             std::cout << "Database is not open" << std::endl;
             throw DatabaseException("Database is not open");
         }
         std::cout << "Returning file stream" << std::endl;
         return db_file_;
     }

const std::string& FileManager::getDatabasePath() const {
    return db_path_;
}

void FileManager::ensureDirectoryExists(const std::string& path) {
    if (!path.empty() && !fs::exists(path)) {
        fs::create_directories(path);
    }
}

void FileManager::initializeMetadata() {
    // 初始化文件头页面（初始换一个页面大小的缓冲区）
    char header_page[PAGE_SIZE] = {0};//用于存储“头文件页面”数据
    PageID page_count = 1; // 只有头页面（页数为1），page_count用于记录数据库当前总页数

    // 将页数信息（二进制心事）写入头页面（缓冲区的固定偏移位置，内存操作）
    std::memcpy(header_page, &page_count, sizeof(PageID));

    // 将缓存区的内容写入文件的起始位置
    db_file_.seekp(0, std::ios::beg);//定位到文件开头
    db_file_.write(header_page, PAGE_SIZE);//写入整个页面数据
    db_file_.flush();//强制刷新磁盘
}

void FileManager::loadMetadata() {

    // 从文件头读取元数据
    // 这里可以扩展为读取更复杂的元数据
}

    //将内存中的元数据持久化到数据库文件中
void FileManager::saveMetadata() {
    // 保存元数据到文件头
    // 这里可以扩展为保存更复杂的元数据

}

} // namespace storage
} // namespace minidb