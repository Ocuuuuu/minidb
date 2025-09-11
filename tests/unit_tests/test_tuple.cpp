#include <../tests/catch2/catch_amalgamated.hpp>
#include <common/Value.h>
#include <common/Tuple.h>

TEST_CASE("Tuple class functionality", "[tuple][unit]") 
{
    std::vector<minidb::Value> values = {
        minidb::Value(42),
        minidb::Value("Hello"),
        minidb::Value(true)
    };
    
    minidb::Tuple tuple(values);
    
    SECTION("Tuple construction") {
        REQUIRE(tuple.getColumnCount() == 3);
    }

    SECTION("Value access") {
        REQUIRE(tuple.getValue(0).getAsInt() == 42); // 使用 getAsInt()
        REQUIRE(tuple.getValue(1).getAsString() == "Hello"); // 使用 getAsString()
        REQUIRE(tuple.getValue(2).getAsBool() == true); // 使用 getAsBool()
    }
    
    SECTION("Out of bounds access") {
        REQUIRE_THROWS(tuple.getValue(5));
    }
}