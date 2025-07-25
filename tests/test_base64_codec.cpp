#include "util/base64_codec.hpp"
#include <catch2/catch_test_macros.hpp>


namespace ws::test {

TEST_CASE("basic usage", "[base64_codec]")
{
    SECTION("to_base64")
    {
        REQUIRE(to_base64("") == "");
        REQUIRE(to_base64("abc") == "YWJj");
        REQUIRE(to_base64("ABC") == "QUJD");
        REQUIRE(to_base64("hello, world") == "aGVsbG8sIHdvcmxk");
    }

    SECTION("from_base64")
    {
        REQUIRE(from_base64("") == "");
        REQUIRE(from_base64("YWJj") == "abc");
        REQUIRE(from_base64("QUJD") == "ABC");
        REQUIRE(from_base64("aGVsbG8sIHdvcmxk") == "hello, world");
    }
}

} // namespace ws::test
