#ifndef MINIDB_FILEMANAGER_H
#define MINIDB_FILEMANAGER_H

#include <string>
#include <fstream>
#include <memory>
#include <filesystem>

namespace minidb {
    namespace storage {

        class FileManager {
        public:
            FileManager();
            ~FileManager();

            FileManager(const FileManager&) = delete;
            FileManager& operator=(const FileManager&) = delete;

            void createDatabase(const std::string& db_name);
            void openDatabase(const std::string& db_name);
            void closeDatabase();
            bool isOpen() const;

            std::fstream& getFileStream();
            const std::string& getDatabasePath() const;

        private:
            std::string db_name_;
            std::string db_path_;
            std::fstream db_file_;
            bool is_open_{false};

            void ensureDirectoryExists(const std::string& path);
        };

    } // namespace storage
} // namespace minidb

#endif // MINIDB_FILEMANAGER_H