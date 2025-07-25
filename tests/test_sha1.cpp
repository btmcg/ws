#include "sha1.hpp"
#include <catch2/catch_test_macros.hpp>


namespace ws::test {

TEST_CASE("SHA-1 basic usage", "[sha1]")
{
    SECTION("empty string")
    {
        REQUIRE(sha1::hash_hex("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    }

    SECTION("single character")
    {
        REQUIRE(sha1::hash_hex("a") == "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8");
    }

    SECTION("test vector abc")
    {
        REQUIRE(sha1::hash_hex("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d");
    }

    SECTION("test vector message digest")
    {
        REQUIRE(sha1::hash_hex("message digest") == "c12252ceda8be8994d5fa0290a47231c1d16aae3");
    }

    SECTION("test vector alphabet")
    {
        std::string const input = "abcdefghijklmnopqrstuvwxyz";
        std::string const expected = "32d10c7b8cf96570ca04ce37f2a19d84240d3a89";
        REQUIRE(sha1::hash_hex(input) == expected);
    }

    SECTION("longer message")
    {
        std::string const input = "The quick brown fox jumps over the lazy dog";
        std::string const expected = "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12";
        REQUIRE(sha1::hash_hex(input) == expected);
    }
}

TEST_CASE("SHA-1 binary data", "[sha1]")
{
    SECTION("binary hash function")
    {
        std::string const text = "abc";
        auto const digest1 = sha1::hash(text);
        auto const digest2
                = sha1::hash(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
        REQUIRE(digest1 == digest2);
    }
}

TEST_CASE("SHA-1 digest properties", "[sha1]")
{
    SECTION("digest size")
    {
        auto const digest = sha1::hash("test");
        REQUIRE(digest.size() == sha1::DIGEST_SIZE);
        REQUIRE(digest.size() == 20);
    }

    SECTION("hex string length")
    {
        std::string const hex = sha1::hash_hex("test");
        REQUIRE(hex.length() == 40); // 20 bytes * 2 hex chars per byte
    }
}

} // namespace test
