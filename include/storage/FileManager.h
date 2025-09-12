//
// Created by tang_ on 2025/9/9.
//

#ifndef MINIDB_FILEMANAGER_H
#define MINIDB_FILEMANAGER_H
#include "common/Constants.h"
#include "common/Exception.h"
#include <string>
//文件流000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
#include <fstream>
#include <unordered_map>
//命名控件嵌套用法
namespace minidb {
    namespace storage {

        class FileManager {
        public:
            //构造函数
            FileManager();

            ~FileManager();

            // 数据库文件管理
            //创建数据库
            void createDatabase(const std::string& db_name);

            void openDatabase(const std::string& db_name);

            void closeDatabase();

            bool isOpen() const;

            // 文件操作
            std::fstream& getFileStream();
            const std::string& getDatabasePath() const;

            // 元数据管理
            void initializeMetadata();
            void loadMetadata();
            void saveMetadata();

        private:
            std::string db_name_;
            std::string db_path_;
            std::fstream db_file_;
            bool is_open_{false};

            // 确保目录存在
            void ensureDirectoryExists(const std::string& path);
        };

    } // namespace storage
} // namespace minidb

#endif //MINIDB_FILEMANAGER_H