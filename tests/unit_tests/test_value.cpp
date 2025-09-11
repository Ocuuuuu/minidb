#include <../tests/catch2/catch_amalgamated.hpp>
#include <common/Value.h>

TEST_CASE("Value class functionality", "[value][unit]")
{
    SECTION("Integer value") {
        minidb::Value int_val(42);
        REQUIRE(int_val.getType() == minidb::TypeId::INTEGER);  // 改为 TypeId::INTEGER
    }

    SECTION("String value") {
        minidb::Value str_val("Hello");
        REQUIRE(str_val.getType() == minidb::TypeId::VARCHAR);  // 改为 TypeId::VARCHAR
    }

    SECTION("Boolean value") {
        minidb::Value bool_val(true);
        REQUIRE(bool_val.getType() == minidb::TypeId::BOOLEAN);  // 改为 TypeId::BOOLEAN
    }
}