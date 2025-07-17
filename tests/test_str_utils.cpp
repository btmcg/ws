#include "str_utils.hpp"
#include <catch2/catch_test_macros.hpp>


TEST_CASE("basic usage", "[str_utils]")
{
    SECTION("to_upper")
    {
        using ws::to_upper;
        REQUIRE(to_upper("") == "");
        REQUIRE(to_upper("abc") == "ABC");
        REQUIRE(to_upper("ABC") == "ABC");
        REQUIRE(to_upper("AbC") == "ABC");
    }

    SECTION("to_lower")
    {
        using ws::to_lower;
        REQUIRE(to_lower("") == "");
        REQUIRE(to_lower("abc") == "abc");
        REQUIRE(to_lower("ABC") == "abc");
        REQUIRE(to_lower("AbC") == "abc");
    }
}
