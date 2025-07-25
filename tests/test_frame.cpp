#include "frame.hpp"
#include <catch2/catch_test_macros.hpp>


TEST_CASE("basic_websocket_header", "[basic_websocket_header]")
{

    SECTION("test defaults")
    {
        ws::basic_websocket_header header{};
        REQUIRE_FALSE(header.fin());
        REQUIRE_FALSE(header.rsv1());
        REQUIRE_FALSE(header.rsv2());
        REQUIRE_FALSE(header.rsv3());
        REQUIRE(header.op_code() == ws::OpCode::Continuation);
        REQUIRE_FALSE(header.masked());
        REQUIRE(header.payload_len_indicator() == 0);
    }
}

TEST_CASE("frame", "[frame]")
{
    SECTION("test defaults and reset")
    {
        ws::frame frame;
        REQUIRE_FALSE(frame.fin());
        REQUIRE_FALSE(frame.rsv1());
        REQUIRE_FALSE(frame.rsv2());
        REQUIRE_FALSE(frame.rsv3());
        REQUIRE(frame.op_code() == ws::OpCode::Continuation);
        REQUIRE_FALSE(frame.masked());
        REQUIRE(frame.payload_len() == 0);
        REQUIRE(frame.header_size() == 0);
        REQUIRE_FALSE(frame.valid());
        REQUIRE(frame.masking_key().size() == 4);
        REQUIRE(frame.total_size() == 0);
        REQUIRE(frame.get_payload_data().empty());
        REQUIRE(frame.get_raw_payload_data().empty());
        REQUIRE_FALSE(frame.get_text_payload());

        frame.reset();
        REQUIRE_FALSE(frame.fin());
        REQUIRE_FALSE(frame.rsv1());
        REQUIRE_FALSE(frame.rsv2());
        REQUIRE_FALSE(frame.rsv3());
        REQUIRE(frame.op_code() == ws::OpCode::Continuation);
        REQUIRE_FALSE(frame.masked());
        REQUIRE(frame.payload_len() == 0);
        REQUIRE(frame.header_size() == 0);
        REQUIRE_FALSE(frame.valid());
        REQUIRE(frame.masking_key().size() == 4);
        REQUIRE(frame.total_size() == 0);
        REQUIRE(frame.get_payload_data().empty());
        REQUIRE(frame.get_raw_payload_data().empty());
        REQUIRE_FALSE(frame.get_text_payload());
    }
}
