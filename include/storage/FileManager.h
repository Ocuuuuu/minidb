#ifndef MINIDB_FILEMANAGER_H
#define MINIDB_FILEMANAGER_H

#include <fstream>
#include <string>

namespace minidb {
    namespace storage {

        class FileManager {
        public:
            FileManager();
            ~FileManager();

            // 数据库操作
            void createDatabase(const std::string& db_name);
            void openDatabase(const std::string& db_name);
            void closeDatabase();
            bool isOpen() const;

            // 数据库信息获取
            std::fstream& getFileStream();
            const std::string& getDatabasePath() const;
            const std::string& getDatabaseName() const;

            // 数据库管理 - 移除了static修饰符
            bool databaseExists(const std::string& db_name);  // 移除static
            void deleteDatabase(const std::string& db_name);  // 移除static
            static bool isValidDatabaseName(const std::string& name);  // 保持static

        private:
            void ensureDirectoryExists(const std::string& path);

        private:
            std::fstream db_file_;
            std::string db_name_;
            std::string db_path_;
            bool is_open_;
        };

    } // namespace storage
} // namespace minidb

#endif // MINIDB_FILEMANAGER_H