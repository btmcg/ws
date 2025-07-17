#include "base64.hpp"
#include <catch2/catch_test_macros.hpp>


TEST_CASE("basic usage", "[base64]")
{
    SECTION("to_upper")
    {
        using ws::to_base64;
        REQUIRE(to_base64("") == "");
        REQUIRE(to_base64("abc") == "YWJj");
        REQUIRE(to_base64("ABC") == "QUJD");
    }
}

