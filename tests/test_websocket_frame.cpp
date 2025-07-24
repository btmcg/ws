#include "websocket_frame.hpp"
#include <catch2/catch_test_macros.hpp>


TEST_CASE("basic_websocket_header", "[basic_websocket_header]")
{
    using namespace ws;

    SECTION("test defaults")
    {
        basic_websocket_header header{};
        REQUIRE_FALSE(header.fin());
        REQUIRE_FALSE(header.rsv1());
        REQUIRE_FALSE(header.rsv2());
        REQUIRE_FALSE(header.rsv3());
        REQUIRE(header.op_code() == OpCode::Continuation);
        REQUIRE_FALSE(header.masked());
        REQUIRE(header.payload_len_indicator() == 0);
    }
}

TEST_CASE("websocket_frame", "[websocket_frame]")
{
    using namespace ws;

    SECTION("test defaults and reset")
    {
        websocket_frame frame;
        REQUIRE_FALSE(frame.fin());
        REQUIRE_FALSE(frame.rsv1());
        REQUIRE_FALSE(frame.rsv2());
        REQUIRE_FALSE(frame.rsv3());
        REQUIRE(frame.op_code() == OpCode::Continuation);
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
        REQUIRE(frame.op_code() == OpCode::Continuation);
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
