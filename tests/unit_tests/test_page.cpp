#include <../tests/catch2/catch_amalgamated.hpp>
#include "storage/Page.h"
#include "common/Exception.h"
#include <cstring>
#include <vector>

using namespace minidb::storage;
using namespace Catch::Matchers;

TEST_CASE("Page Construction and Initialization", "[page][construction]") {
    SECTION("Default constructor should initialize correctly") {
        Page page;
        
        REQUIRE(page.getPageId() == minidb::INVALID_PAGE_ID);
        REQUIRE(page.getPageType() == PageType::DATA_PAGE);
        REQUIRE(page.getFreeSpace() == minidb::PAGE_SIZE - sizeof(PageHeader));
        REQUIRE(page.getPinCount() == 0);
        REQUIRE_FALSE(page.isDirty());
        REQUIRE_FALSE(page.isPinned());
    }
    
    SECTION("Constructor with page ID should initialize correctly") {
        Page page(123);
        
        REQUIRE(page.getPageId() == 123);
        REQUIRE(page.getPageType() == PageType::DATA_PAGE);
        REQUIRE(page.getFreeSpace() == minidb::PAGE_SIZE - sizeof(PageHeader));
        REQUIRE(page.getPinCount() == 0);
        REQUIRE_FALSE(page.isDirty());
        REQUIRE_FALSE(page.isPinned());
    }
}

TEST_CASE("Page Pin Count Management", "[page][pin]") {
    Page page;
    
    SECTION("Increment pin count should work correctly") {
        page.incrementPinCount();
        REQUIRE(page.getPinCount() == 1);
        REQUIRE(page.isPinned());
        
        page.incrementPinCount();
        REQUIRE(page.getPinCount() == 2);
        REQUIRE(page.isPinned());
    }
    
    SECTION("Decrement pin count should work correctly") {
        page.incrementPinCount();
        page.incrementPinCount();
        
        page.decrementPinCount();
        REQUIRE(page.getPinCount() == 1);
        REQUIRE(page.isPinned());
        
        page.decrementPinCount();
        REQUIRE(page.getPinCount() == 0);
        REQUIRE_FALSE(page.isPinned());
    }
    
    SECTION("Decrement pin count should not go below zero") {
        page.decrementPinCount(); // Should not throw or go negative
        REQUIRE(page.getPinCount() == 0);
        REQUIRE_FALSE(page.isPinned());
    }
}

TEST_CASE("Page Dirty Flag Management", "[page][dirty]") {
    Page page;
    
    SECTION("Set dirty flag should work correctly") {
        page.setDirty(true);
        REQUIRE(page.isDirty());
        
        page.setDirty(false);
        REQUIRE_FALSE(page.isDirty());
    }
}

TEST_CASE("Page Space Checking", "[page][space]") {
    Page page;
    const uint16_t total_space = minidb::PAGE_SIZE - sizeof(PageHeader);
    
    SECTION("Initial free space should be correct") {
        REQUIRE(page.getFreeSpace() == total_space);
    }
    
    SECTION("Has enough space should work correctly") {
        // Should have space for records that fit
        REQUIRE(page.hasEnoughSpace(100));
        REQUIRE(page.hasEnoughSpace(total_space - 4)); // -4 for slot overhead
        
        // Should not have space for records that are too big
        REQUIRE_FALSE(page.hasEnoughSpace(total_space - 3)); // Would need +4 for slot
        REQUIRE_FALSE(page.hasEnoughSpace(total_space + 1));
    }
}

TEST_CASE("Page Record Insertion", "[page][insert]") {
    Page page(1);
    const char* test_data = "Hello, World!";
    const uint16_t data_size = strlen(test_data) + 1; // Include null terminator
    
    SECTION("Insert single record should work") {
        minidb::RID rid;
        bool result = page.insertRecord(test_data, data_size, &rid);
        
        REQUIRE(result);
        REQUIRE(rid.page_id == 1);
        REQUIRE(rid.slot_num == 0);
        REQUIRE(page.getFreeSpace() == (minidb::PAGE_SIZE - sizeof(PageHeader) - data_size - 4));
        REQUIRE(page.isDirty());
    }
    
    SECTION("Insert multiple records should work") {
        minidb::RID rid1, rid2;
        
        REQUIRE(page.insertRecord("Record1", 8, &rid1));
        REQUIRE(rid1.slot_num == 0);
        
        REQUIRE(page.insertRecord("Record2", 8, &rid2));
        REQUIRE(rid2.slot_num == 1);
        
        REQUIRE(page.getFreeSpace() == (minidb::PAGE_SIZE - sizeof(PageHeader) - 16 - 8)); // 16 for slots, 16 for data
    }
    
    SECTION("Insert should fail when not enough space") {
        char large_data[minidb::PAGE_SIZE] = {0}; // Too large
        
        bool result = page.insertRecord(large_data, minidb::PAGE_SIZE, nullptr);
        REQUIRE_FALSE(result);
    }
    
    SECTION("Insert without RID should work") {
        bool result = page.insertRecord(test_data, data_size, nullptr);
        REQUIRE(result);
    }
}

TEST_CASE("Page Record Retrieval", "[page][get]") {
    Page page(1);
    const char* test_data = "Test record data";
    const uint16_t data_size = strlen(test_data) + 1;
    
    SECTION("Get existing record should work") {
        minidb::RID rid;
        page.insertRecord(test_data, data_size, &rid);
        
        char buffer[100];
        uint16_t retrieved_size;
        bool result = page.getRecord(rid, buffer, &retrieved_size);
        
        REQUIRE(result);
        REQUIRE(retrieved_size == data_size);
        REQUIRE(strcmp(buffer, test_data) == 0);
    }
    
    SECTION("Get record without size should work") {
        minidb::RID rid;
        page.insertRecord(test_data, data_size, &rid);
        
        char buffer[100];
        bool result = page.getRecord(rid, buffer, nullptr);
        
        REQUIRE(result);
        REQUIRE(strcmp(buffer, test_data) == 0);
    }
    
    SECTION("Get non-existent record should fail") {
        minidb::RID invalid_rid{999, 999};
        char buffer[100];
        
        bool result = page.getRecord(invalid_rid, buffer, nullptr);
        REQUIRE_FALSE(result);
    }
    
    SECTION("Get record with wrong page ID should fail") {
        minidb::RID rid;
        page.insertRecord(test_data, data_size, &rid);
        
        minidb::RID wrong_page_rid{999, rid.slot_num};
        char buffer[100];
        
        bool result = page.getRecord(wrong_page_rid, buffer, nullptr);
        REQUIRE_FALSE(result);
    }
}

TEST_CASE("Page Record Deletion", "[page][delete]") {
    Page page(1);
    const char* test_data = "Data to delete";
    const uint16_t data_size = strlen(test_data) + 1;
    
    SECTION("Delete existing record should work") {
        minidb::RID rid;
        page.insertRecord(test_data, data_size, &rid);
        
        bool result = page.deleteRecord(rid);
        REQUIRE(result);
        REQUIRE(page.isDirty());
        
        // Should not be able to get deleted record
        char buffer[100];
        REQUIRE_FALSE(page.getRecord(rid, buffer, nullptr));
    }
    
    SECTION("Delete non-existent record should fail") {
        minidb::RID invalid_rid{999, 999};
        bool result = page.deleteRecord(invalid_rid);
        REQUIRE_FALSE(result);
    }
}

TEST_CASE("Page Record Update", "[page][update]") {
    Page page(1);
    const char* old_data = "Old data";
    const char* new_data = "New updated data";
    const uint16_t old_size = strlen(old_data) + 1;
    const uint16_t new_size = strlen(new_data) + 1;
    
    SECTION("Update existing record should work") {
        minidb::RID old_rid;
        page.insertRecord(old_data, old_size, &old_rid);
        
        minidb::RID new_rid;
        bool result = page.updateRecord(old_rid, new_data, new_size, &new_rid);
        
        REQUIRE(result);
        REQUIRE(new_rid.page_id == 1);
        REQUIRE(new_rid.slot_num == 1); // Should be new slot
        
        // Old record should be deleted
        char buffer[100];
        REQUIRE_FALSE(page.getRecord(old_rid, buffer, nullptr));
        
        // New record should be accessible
        REQUIRE(page.getRecord(new_rid, buffer, nullptr));
        REQUIRE(strcmp(buffer, new_data) == 0);
    }
    
    SECTION("Update without new RID should work") {
        minidb::RID old_rid;
        page.insertRecord(old_data, old_size, &old_rid);
        
        bool result = page.updateRecord(old_rid, new_data, new_size, nullptr);
        REQUIRE(result);
    }
    
    SECTION("Update non-existent record should throw") {
        minidb::RID invalid_rid{999, 999};
        
        REQUIRE_THROWS_AS(
            page.updateRecord(invalid_rid, new_data, new_size, nullptr),
            minidb::InvalidRIDException
        );
    }
}

TEST_CASE("Page Serialization and Deserialization", "[page][serialize]") {
    Page original_page(1);
    
    // Insert some test data
    minidb::RID rid1, rid2;
    original_page.insertRecord("First record", 13, &rid1);
    original_page.insertRecord("Second record", 14, &rid2);
    original_page.setDirty(true);
    original_page.incrementPinCount();
    
    SECTION("Serialization should preserve data") {
        char buffer[minidb::PAGE_SIZE];
        original_page.serialize(buffer);
        
        Page restored_page;
        restored_page.deserialize(buffer);
        
        // Check basic properties
        REQUIRE(restored_page.getPageId() == original_page.getPageId());
        REQUIRE(restored_page.getPageType() == original_page.getPageType());
        REQUIRE(restored_page.getFreeSpace() == original_page.getFreeSpace());
        REQUIRE(restored_page.getHeader().slot_count == original_page.getHeader().slot_count);

        // Note: pin_count is not serialized as it's runtime state
        REQUIRE(restored_page.getPinCount() == 0);

        // is_dirty SHOULD be preserved through serialization
        REQUIRE(restored_page.isDirty() == original_page.isDirty());

        // Check that records are preserved
        char data1[100], data2[100];
        REQUIRE(restored_page.getRecord(rid1, data1, nullptr));
        REQUIRE(restored_page.getRecord(rid2, data2, nullptr));

        REQUIRE(strcmp(data1, "First record") == 0);
        REQUIRE(strcmp(data2, "Second record") == 0);
    }

    SECTION("Round-trip serialization should work") {
        // Serialize original
        char buffer[minidb::PAGE_SIZE];
        original_page.serialize(buffer);

        // Create new page and deserialize
        Page new_page;
        new_page.deserialize(buffer);

        // Serialize again and compare
        char buffer2[minidb::PAGE_SIZE];
        new_page.serialize(buffer2);

        REQUIRE(memcmp(buffer, buffer2, minidb::PAGE_SIZE) == 0);
    }
}

TEST_CASE("Page Slot Operations", "[page][slot]") {
    Page page(1);

    SECTION("Invalid slot access through public methods should fail") {
        minidb::RID invalid_rid{1, 999}; // 无效槽位
        char buffer[100];
        uint16_t size;

        // 通过 getRecord 测试无效槽位
        REQUIRE_FALSE(page.getRecord(invalid_rid, buffer, nullptr));
        REQUIRE_FALSE(page.getRecord(invalid_rid, buffer, &size));

        // 通过 deleteRecord 测试无效槽位
        REQUIRE_FALSE(page.deleteRecord(invalid_rid));

        // 通过 updateRecord 测试无效槽位（应该抛出异常）
        REQUIRE_THROWS_AS(
            page.updateRecord(invalid_rid, "new data", 8, nullptr),
            minidb::InvalidRIDException
        );
    }

    SECTION("Valid slot operations should work") {
        // 插入记录来创建有效槽位
        minidb::RID rid;
        const char* test_data = "Test";
        const uint16_t data_size = 5; // 包括null终止符

        REQUIRE(page.insertRecord(test_data, data_size, &rid));

        // 测试通过公共接口访问有效槽位
        char buffer[100];
        uint16_t retrieved_size;

        REQUIRE(page.getRecord(rid, buffer, &retrieved_size));
        REQUIRE(retrieved_size == data_size);
        REQUIRE(strcmp(buffer, test_data) == 0);

        // 测试删除有效槽位
        REQUIRE(page.deleteRecord(rid));
        REQUIRE_FALSE(page.getRecord(rid, buffer, nullptr));

        // 偏移量应该在数据区内（通过序列化数据间接验证）
        char serialized_data[minidb::PAGE_SIZE];
        page.serialize(serialized_data);

        // 可以检查序列化后的数据来间接验证内部结构
        // 但通常不需要直接测试私有方法的行为
    }
}

TEST_CASE("Page Compactify", "[page][compactify]") {
    Page page(1);

    SECTION("Compactify is not implemented") {
        // 由于 compactify 是私有方法，我们无法直接测试
        // 这个测试用例可以删除，或者通过其他方式间接测试
        // 例如，测试页面在删除记录后的行为
        SUCCEED("Compactify is a private method and cannot be tested directly");
    }

    SECTION("Test page behavior after record deletion") {
        // 插入几条记录
        minidb::RID rid1, rid2;
        page.insertRecord("Record1", 8, &rid1);
        page.insertRecord("Record2", 8, &rid2);

        uint16_t initial_free_space = page.getFreeSpace();

        // 删除一条记录
        REQUIRE(page.deleteRecord(rid1));

        // 检查空间没有立即回收（因为compactify未实现）
        REQUIRE(page.getFreeSpace() == initial_free_space);

        // 但记录应该被标记为删除
        char buffer[100];
        REQUIRE_FALSE(page.getRecord(rid1, buffer, nullptr));

        // 另一条记录应该仍然存在
        REQUIRE(page.getRecord(rid2, buffer, nullptr));
        REQUIRE(strcmp(buffer, "Record2") == 0);
    }
}

TEST_CASE("Page ToString", "[page][tostring]") {
    Page page(123);
    page.setPageType(PageType::INDEX_PAGE);

    std::string str = page.toString();

    REQUIRE(str.find("ID: 123") != std::string::npos);
    REQUIRE(str.find("Type: 1") != std::string::npos); // INDEX_PAGE value
    REQUIRE(str.find("Slot Count: 0") != std::string::npos);
    REQUIRE(str.find("Free Space:") != std::string::npos);
    REQUIRE(str.find("Dirty: false") != std::string::npos);
}

TEST_CASE("Page Data Access", "[page][data]") {
    Page page(1);

    SECTION("Get data should return valid pointers") {
        char* writable_data = page.getData();
        const char* readable_data = page.getData();

        REQUIRE(writable_data != nullptr);
        REQUIRE(readable_data != nullptr);
        REQUIRE(writable_data == readable_data);
    }

    SECTION("Header access should work") {
        const PageHeader& const_header = page.getHeader();
        PageHeader& mutable_header = page.getHeader();

        REQUIRE(const_header.page_id == 1);
        REQUIRE(mutable_header.page_id == 1);

        // Test that we can modify through mutable reference
        mutable_header.page_type = PageType::META_PAGE;
        REQUIRE(page.getPageType() == PageType::META_PAGE);
    }
}

TEST_CASE("Page Boundary Conditions", "[page][boundary]") {
    Page page(1);
    const uint16_t max_record_size = page.getFreeSpace() - 4; // Account for slot overhead

    SECTION("Insert maximum size record should work") {
        std::vector<char> large_data(max_record_size, 'A');
        large_data.back() = '\0'; // Null terminate

        bool result = page.insertRecord(large_data.data(), max_record_size, nullptr);
        REQUIRE(result);
        REQUIRE(page.getFreeSpace() == 0);
    }

    SECTION("Insert slightly too large record should fail") {
        std::vector<char> too_large_data(max_record_size + 1, 'A');

        bool result = page.insertRecord(too_large_data.data(), max_record_size + 1, nullptr);
        REQUIRE_FALSE(result);
    }
}

// 添加一个辅助函数来获取槽位数量（如果需要的话）
uint16_t getSlotCount(const Page& page) {
    return page.getHeader().slot_count;
}

TEST_CASE("Page Slot Count Access", "[page][slotcount]") {
    Page page(1);

    SECTION("Initial slot count should be zero") {
        REQUIRE(getSlotCount(page) == 0);
    }

    SECTION("Slot count should increase after insert") {
        page.insertRecord("Test", 5, nullptr);
        REQUIRE(getSlotCount(page) == 1);

        page.insertRecord("Test2", 6, nullptr);
        REQUIRE(getSlotCount(page) == 2);
    }
}