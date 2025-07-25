#include "byte_buffer.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring> // std::memcmp, std::memcpy


namespace ws::test {

TEST_CASE("basic usage", "[byte_buffer]")
{
    byte_buffer<10> buf;

    SECTION("initial state")
    {
        REQUIRE(buf.capacity() == 10);
        REQUIRE(buf.read_ptr() != nullptr);
        REQUIRE(buf.write_ptr() != nullptr);
        REQUIRE(buf.bytes_unread() == 0);
        REQUIRE(buf.bytes_left() == 10);
    }

    SECTION("move constructor")
    {
        std::memcpy(buf.write_ptr(), "abcde", sizeof("abcde") - 1);
        buf.bytes_written(sizeof("abcde") - 1);
        byte_buffer<10> moved = std::move(buf);
        REQUIRE(moved.bytes_unread() == 5);
        REQUIRE(moved.bytes_left() == 5);
    }

    SECTION("move assignment")
    {
        std::memcpy(buf.write_ptr(), "abcde", sizeof("abcde") - 1);
        buf.bytes_written(sizeof("abcde") - 1);

        byte_buffer<10> moved;
        moved.bytes_written(3); // dummy data

        moved = std::move(buf);
        REQUIRE(moved.bytes_unread() == 5);
        REQUIRE(moved.bytes_left() == 5);
    }

    SECTION("shift on empty buffer")
    {
        std::uint8_t const* rptr = buf.read_ptr();
        std::uint8_t* wptr = buf.write_ptr();
        REQUIRE(buf.capacity() == 10);
        REQUIRE(buf.bytes_unread() == 0);
        REQUIRE(buf.bytes_left() == 10);

        REQUIRE(buf.shift() == 0);
        REQUIRE(buf.capacity() == 10);
        REQUIRE(buf.bytes_unread() == 0);
        REQUIRE(buf.bytes_left() == 10);
        REQUIRE(buf.read_ptr() == rptr);
        REQUIRE(buf.write_ptr() == wptr);
    }

    SECTION("read write")
    {
        std::memcpy(buf.write_ptr(), "abcde", sizeof("abcde") - 1);
        buf.bytes_written(sizeof("abcde") - 1);
        REQUIRE(buf.bytes_unread() == 5);
        REQUIRE(buf.bytes_left() == 5);

        REQUIRE(std::memcmp(buf.read_ptr(), "abc", 3) == 0);
        buf.bytes_read(3);
        REQUIRE(buf.bytes_unread() == 2);
        REQUIRE(buf.bytes_left() == 5);

        std::memcpy(buf.write_ptr(), "12345", sizeof("12345") - 1);
        buf.bytes_written(sizeof("12345") - 1);
        REQUIRE(buf.bytes_unread() == 7);
        REQUIRE(buf.bytes_left() == 0);

        REQUIRE(std::memcmp(buf.read_ptr(), "de123", 5) == 0);
        buf.bytes_read(5);
        REQUIRE(buf.bytes_unread() == 2);
        REQUIRE(buf.bytes_left() == 0);

        REQUIRE(std::memcmp(buf.read_ptr(), "45", 2) == 0);
        buf.bytes_read(2);
        REQUIRE(buf.bytes_unread() == 0);
        REQUIRE(buf.bytes_left() == 0);

        REQUIRE(buf.read_ptr() == buf.write_ptr());
    }

    SECTION("shift")
    {
        std::memcpy(buf.write_ptr(), "abcdefghij", sizeof("abcdefghij") - 1);
        buf.bytes_written(sizeof("abcdefghij") - 1);
        REQUIRE(buf.bytes_unread() == 10);
        REQUIRE(buf.bytes_left() == 0);

        REQUIRE(std::memcmp(buf.read_ptr(), "abcdefg", 7) == 0);
        buf.bytes_read(7);
        REQUIRE(buf.bytes_unread() == 3);
        REQUIRE(buf.bytes_left() == 0);

        REQUIRE(buf.shift() == 3);

        REQUIRE(buf.bytes_unread() == 3);
        REQUIRE(buf.bytes_left() == 7);

        REQUIRE(std::memcmp(buf.read_ptr(), "hij", 3) == 0);
        buf.bytes_read(3);
        REQUIRE(buf.bytes_unread() == 0);
        REQUIRE(buf.bytes_left() == 7);

        std::memcpy(buf.write_ptr(), "1234", sizeof("1234") - 1);
        buf.bytes_written(sizeof("1234") - 1);
        REQUIRE(buf.bytes_unread() == 4);
        REQUIRE(buf.bytes_left() == 3);

        REQUIRE(std::memcmp(buf.read_ptr(), "1234", 4) == 0);
        buf.bytes_read(4);
        REQUIRE(buf.bytes_unread() == 0);
        REQUIRE(buf.bytes_left() == 3);

        REQUIRE(buf.read_ptr() == buf.write_ptr());
    }
}

} // namespace test
