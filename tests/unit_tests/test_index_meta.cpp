#include <../tests/catch2/catch_amalgamated.hpp>
#include "engine/catalog/index_meta.h"
#include "common/Exception.h"
#include <cstring>

TEST_CASE("IndexMeta Construction and Basic Properties", "[catalog][index_meta]") {
    SECTION("Default constructor creates invalid index") {
        minidb::catalog::IndexMeta meta;
        
        REQUIRE_FALSE(meta.is_valid());
        REQUIRE(meta.get_index_name().empty());
        REQUIRE(meta.get_table_name().empty());
        REQUIRE(meta.get_column_name().empty());
        REQUIRE(meta.get_key_type() == minidb::TypeId::INVALID);
        REQUIRE(meta.get_root_page_id() == minidb::INVALID_PAGE_ID);
    }
    
    SECTION("Parameterized constructor with valid data") {
        minidb::catalog::IndexMeta meta("student_id_idx", "students", "id", 
                                       minidb::TypeId::INTEGER, 123);
        
        REQUIRE(meta.is_valid());
        REQUIRE(meta.get_index_name() == "student_id_idx");
        REQUIRE(meta.get_table_name() == "students");
        REQUIRE(meta.get_column_name() == "id");
        REQUIRE(meta.get_key_type() == minidb::TypeId::INTEGER);
        REQUIRE(meta.get_root_page_id() == 123);
    }
    
    SECTION("Constructor with empty index name throws exception") {
        REQUIRE_THROWS_AS(
            minidb::catalog::IndexMeta("", "students", "id", minidb::TypeId::INTEGER),
            minidb::DatabaseException
        );
    }
    
    SECTION("Constructor with empty table name throws exception") {
        REQUIRE_THROWS_AS(
            minidb::catalog::IndexMeta("student_id_idx", "", "id", minidb::TypeId::INTEGER),
            minidb::DatabaseException
        );
    }
    
    SECTION("Constructor with empty column name throws exception") {
        REQUIRE_THROWS_AS(
            minidb::catalog::IndexMeta("student_id_idx", "students", "", minidb::TypeId::INTEGER),
            minidb::DatabaseException
        );
    }
    
    SECTION("Constructor with invalid key type throws exception") {
        REQUIRE_THROWS_AS(
            minidb::catalog::IndexMeta("student_id_idx", "students", "id", minidb::TypeId::INVALID),
            minidb::DatabaseException
        );
    }
}

TEST_CASE("IndexMeta Serialization and Deserialization", "[catalog][index_meta][serialization]") {
    SECTION("Serialization round-trip preserves data") {
        minidb::catalog::IndexMeta original("user_email_idx", "users", "email", 
                                          minidb::TypeId::VARCHAR, 456);
        
        // 序列化
        size_t buffer_size = minidb::catalog::IndexMeta::get_serialized_size();
        char* buffer = new char[buffer_size];
        original.serialize(buffer);
        
        // 反序列化
        minidb::catalog::IndexMeta restored;
        restored.deserialize(buffer);
        delete[] buffer;
        
        // 验证数据一致性
        REQUIRE(restored.is_valid());
        REQUIRE(restored.get_index_name() == original.get_index_name());
        REQUIRE(restored.get_table_name() == original.get_table_name());
        REQUIRE(restored.get_column_name() == original.get_column_name());
        REQUIRE(restored.get_key_type() == original.get_key_type());
        REQUIRE(restored.get_root_page_id() == original.get_root_page_id());
    }
    
    SECTION("Serialization with long names gets truncated properly") {
        std::string long_name(100, 'x'); // 创建100个字符的字符串
        minidb::catalog::IndexMeta original(long_name, long_name, long_name, 
                                          minidb::TypeId::INTEGER, 789);
        
        size_t buffer_size = minidb::catalog::IndexMeta::get_serialized_size();
        char* buffer = new char[buffer_size];
        original.serialize(buffer);
        
        minidb::catalog::IndexMeta restored;
        restored.deserialize(buffer);
        delete[] buffer;
        
        // 名称应该被截断到 NAME_LENGTH-1 个字符
        REQUIRE(restored.get_index_name().length() < long_name.length());
        REQUIRE(restored.get_table_name().length() < long_name.length());
        REQUIRE(restored.get_column_name().length() < long_name.length());
        REQUIRE(restored.is_valid());
    }
    
    SECTION("Serialization with null buffer throws exception") {
        minidb::catalog::IndexMeta meta("test_idx", "test_table", "test_col", 
                                      minidb::TypeId::INTEGER);
        
        REQUIRE_THROWS_AS(meta.serialize(nullptr), minidb::DatabaseException);
        REQUIRE_THROWS_AS(meta.deserialize(nullptr), minidb::DatabaseException);
    }
    
    SECTION("Serialized size is consistent and correct") {
        // 使用 get_serialized_size() 而不是直接访问私有成员
        size_t serialized_size = minidb::catalog::IndexMeta::get_serialized_size();

        // 验证不同实例的序列化大小相同
        minidb::catalog::IndexMeta meta1("idx1", "table1", "col1", minidb::TypeId::INTEGER);
        minidb::catalog::IndexMeta meta2("very_long_index_name_here", "table", "col",
                                       minidb::TypeId::VARCHAR);

        REQUIRE(meta1.get_serialized_size() == meta2.get_serialized_size());
        REQUIRE(meta1.get_serialized_size() == serialized_size);
    }

}

TEST_CASE("IndexMeta String Representation", "[catalog][index_meta][tostring]") {
    SECTION("ToString includes all relevant information") {
        minidb::catalog::IndexMeta meta("product_name_idx", "products", "name", 
                                      minidb::TypeId::VARCHAR, 1024);
        
        std::string str = meta.to_string();
        
        REQUIRE(str.find("product_name_idx") != std::string::npos);
        REQUIRE(str.find("products") != std::string::npos);
        REQUIRE(str.find("name") != std::string::npos);

        // 修正：将逻辑或操作拆分成两个独立的 REQUIRE 语句
        bool has_varchar_name = (str.find("VARCHAR") != std::string::npos);
        bool has_varchar_id = (str.find(std::to_string(static_cast<int>(minidb::TypeId::VARCHAR))) != std::string::npos);
        REQUIRE((has_varchar_name || has_varchar_id)); // 使用括号包装逻辑表达式

        REQUIRE(str.find("1024") != std::string::npos);
    }
}

TEST_CASE("IndexMeta Validity Checking", "[catalog][index_meta][validation]") {
    SECTION("Valid index meta returns true for is_valid()") {
        minidb::catalog::IndexMeta meta("valid_idx", "valid_table", "valid_col", 
                                      minidb::TypeId::INTEGER, 1);
        REQUIRE(meta.is_valid());
    }
    
    SECTION("Index meta with invalid page ID is still valid") {
        minidb::catalog::IndexMeta meta("valid_idx", "valid_table", "valid_col", 
                                      minidb::TypeId::INTEGER, minidb::INVALID_PAGE_ID);
        REQUIRE(meta.is_valid()); // 页面ID无效不影响元数据本身的有效性
    }
    
    SECTION("Index meta with empty names is invalid") {
        // 使用 REQUIRE_THROWS_WITH 来检查抛出的异常和消息
        REQUIRE_THROWS_WITH(
            minidb::catalog::IndexMeta("", "table", "col", minidb::TypeId::INTEGER),
            "[Database] Index name cannot be empty"
        );

        REQUIRE_THROWS_WITH(
            minidb::catalog::IndexMeta("idx", "", "col", minidb::TypeId::INTEGER),
            "[Database] Table name cannot be empty" // 根据实际错误消息调整
        );

        REQUIRE_THROWS_WITH(
            minidb::catalog::IndexMeta("idx", "table", "", minidb::TypeId::INTEGER),
            "[Database] Column name cannot be empty" // 根据实际错误消息调整
        );
    }

    SECTION("Index meta with invalid key type is invalid") {
        REQUIRE_THROWS_WITH(
            minidb::catalog::IndexMeta("idx", "table", "col", minidb::TypeId::INVALID, 1),
            "[Database] Key type cannot be invalid"
        );
    }
}

TEST_CASE("IndexMeta Root Page ID Management", "[catalog][index_meta][root_page]") {
    SECTION("Setting root page ID updates the value") {
        minidb::catalog::IndexMeta meta("test_idx", "test_table", "test_col", 
                                      minidb::TypeId::INTEGER, 100);
        
        REQUIRE(meta.get_root_page_id() == 100);
        
        meta.set_root_page_id(200);
        REQUIRE(meta.get_root_page_id() == 200);
        
        meta.set_root_page_id(minidb::INVALID_PAGE_ID);
        REQUIRE(meta.get_root_page_id() == minidb::INVALID_PAGE_ID);
    }
    
    SECTION("Root page ID is preserved during serialization") {
        minidb::catalog::IndexMeta original("idx", "table", "col", 
                                          minidb::TypeId::INTEGER, 999);
        
        size_t buffer_size = minidb::catalog::IndexMeta::get_serialized_size();
        char* buffer = new char[buffer_size];
        original.serialize(buffer);
        
        minidb::catalog::IndexMeta restored;
        restored.deserialize(buffer);
        delete[] buffer;
        
        REQUIRE(restored.get_root_page_id() == 999);
    }
}

TEST_CASE("IndexMeta Validity Checking") {
    SECTION("Valid metadata but not ready for use") {
        minidb::catalog::IndexMeta meta("idx", "table", "col", minidb::TypeId::INTEGER);
        REQUIRE(meta.is_valid());           // ✅ 元数据有效
        REQUIRE_FALSE(meta.is_ready_for_use()); // ✅ 但尚未准备好使用
    }

    SECTION("Valid and ready for use") {
        minidb::catalog::IndexMeta meta("idx", "table", "col", minidb::TypeId::INTEGER, 123);
        REQUIRE(meta.is_valid());        // ✅ 元数据有效
        REQUIRE(meta.is_ready_for_use()); // ✅ 准备好使用
    }
}