#include "common/Value.h"
#include "common/Tuple.h"
#include "storage/FileManager.h"
#include "storage/DiskManager.h"
#include "storage/BufferManager.h"
#include "storage/Page.h"
#include <iostream>

// ���Ի�������
void testBasicTypes() {
    std::cout << "=== Testing Basic Types ===" << std::endl;

    minidb::Value int_val(42);
    minidb::Value str_val("Hello, MiniDB!");
    minidb::Value bool_val(true);

    std::cout << "Int value: " << int_val.toString() << std::endl;
    std::cout << "String value: " << str_val.toString() << std::endl;
    std::cout << "Bool value: " << bool_val.toString() << std::endl;

    // ���� Tuple
    std::vector<minidb::Value> values = {int_val, str_val, bool_val};
    minidb::Tuple tuple(values);

    std::cout << "Tuple: " << tuple.toString() << std::endl;
    std::cout << "Tuple size: " << tuple.getColumnCount() << std::endl;
}

// ���Դ洢ϵͳ
void testStorageSystem() {
    std::cout << "\n=== Testing Storage System ===" << std::endl;

    try {
        // �����ļ�������
        minidb::storage::FileManager file_manager;
        std::cout << "FileManager created." << std::endl;

        // �������ݿ�
        file_manager.createDatabase("testdb");
        std::cout << "Database created." << std::endl;

        // �������̹�����
        minidb::storage::DiskManager disk_manager(&file_manager);
        std::cout << "DiskManager created. Page count: " << disk_manager.getPageCount() << std::endl;

        // ����������������
        minidb::storage::BufferManager buffer_manager(&disk_manager);
        std::cout << "BufferManager created." << std::endl;

        // ������ҳ��
        minidb::PageID page_id = disk_manager.allocatePage();
        std::cout << "Allocated page: " << page_id << std::endl;

        // ��ȡҳ��
        minidb::storage::Page* page = buffer_manager.fetchPage(page_id);
        std::cout << "Fetched page: " << page->toString() << std::endl;

        // ����һЩ��������
        std::string test_data = "Hello, MiniDB!";
        minidb::RID rid;
        if (page->insertRecord(test_data.c_str(), test_data.size(), &rid)) {
            std::cout << "Inserted record with RID: " << rid.toString() << std::endl;
        } else {
            std::cout << "Failed to insert record." << std::endl;
        }

        // ��ȡ��¼
        char buffer[100];
        uint16_t size;
        if (page->getRecord(rid, buffer, &size)) {
            buffer[size] = '\0';
            std::cout << "Read record: " << buffer << std::endl;
        } else {
            std::cout << "Failed to read record." << std::endl;
        }

        // ���ҳ��Ϊ��ҳ
        buffer_manager.unpinPage(page_id, true);
        std::cout << "Page marked as dirty." << std::endl;

        // ˢ������ҳ�浽����
        buffer_manager.flushAllPages();
        std::cout << "All pages flushed to disk." << std::endl;

        // �ر����ݿ�
        file_manager.closeDatabase();
        std::cout << "Database closed." << std::endl;

    } catch (const minidb::DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Standard error: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "Unknown error occurred!" << std::endl;
        throw;
    }
}

int main() {
    try {
        std::cout << "Starting MiniDB test..." << std::endl;

        testBasicTypes();
        testStorageSystem();

        std::cout << "\nAll tests completed successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught in main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught in main!" << std::endl;
        return 1;
    }
}