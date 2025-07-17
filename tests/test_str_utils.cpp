#include "str_utils.hpp"
#include <catch2/catch.hpp>
#include <cstdint>
#include <cstring> // std::memcmp, std::memcpy

TEST_CASE("basic usage", "[str_utils]")
{
    SECTION("to_upper")
    {
        REQUIRE(to_upper("abc") == "ABC");
    }
}

