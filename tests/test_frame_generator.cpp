#include "frame.hpp"
#include "frame_generator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <bit>  // std::byteswap
#include <span> // std::byteswap


namespace ws::test {

/// help fn to convert string data into a uint8_t span as needed by
/// the frame_generator
template <typename T = std::uint8_t const>
std::span<T>
as_uint8_span(auto& str)
{
    return std::span<T>(reinterpret_cast<T*>(str.data()), str.size());
}

template <typename T = std::uint8_t const>
std::string
as_str(std::span<T> sp)
{
    return std::string(reinterpret_cast<char const*>(sp.data()), sp.size());
}


TEST_CASE("frame_generator", "[frame_generator]")
{
    static constexpr std::size_t UnmaskedHeaderSize = 2;
    static constexpr std::size_t MaskedHeaderSize = 6;

    SECTION("ping")
    {
        SECTION("unmasked without payload")
        {
            auto out_frame = ws::frame_generator{}.ping();

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Ping);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
        }

        SECTION("masked without payload")
        {
            auto out_frame = ws::frame_generator{}.ping({}, /*masked=*/true);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Ping);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == MaskedHeaderSize);
        }

        SECTION("unmasked with payload")
        {
            std::string const payload = "this is the payload";
            auto out_frame = ws::frame_generator{}.ping(as_uint8_span(payload));

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Ping);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == payload.size());
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
        }

        SECTION("masked with payload")
        {
            std::string const payload = "this is the payload";
            auto out_frame = ws::frame_generator{}.ping(as_uint8_span(payload), /*masked=*/true);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Ping);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == payload.size());
            REQUIRE(frame.header_size() == MaskedHeaderSize);
            REQUIRE(as_str(frame.get_payload_data()) == payload);
        }
    }

    SECTION("pong")
    {
        SECTION("unmasked without payload")
        {
            auto out_frame = ws::frame_generator{}.pong();

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Pong);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
        }

        SECTION("masked without payload")
        {
            auto out_frame = ws::frame_generator{}.pong({}, /*masked=*/true);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Pong);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == MaskedHeaderSize);
        }

        SECTION("unmasked with payload")
        {
            std::string const payload = "this is the payload";
            auto out_frame = ws::frame_generator{}.pong(as_uint8_span(payload));

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Pong);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == payload.size());
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
        }

        SECTION("masked with payload")
        {
            std::string const payload = "this is the payload";
            auto out_frame = ws::frame_generator{}.pong(as_uint8_span(payload), /*masked=*/true);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Pong);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == payload.size());
            REQUIRE(frame.header_size() == MaskedHeaderSize);
            REQUIRE(as_str(frame.get_payload_data()) == payload);
        }
    }

    SECTION("close")
    {
        std::uint16_t status_code = 0;

        SECTION("unmasked, default status code")
        {
            auto out_frame = ws::frame_generator{}.close();
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize + sizeof(status_code));

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Close);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == sizeof(status_code));
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            std::memcpy(reinterpret_cast<void*>(&status_code),
                    reinterpret_cast<void const*>(frame.get_payload_data().data()),
                    sizeof(status_code));
            status_code = std::byteswap(status_code);
            REQUIRE(status_code == 1000);
        }

        SECTION("masked, default status code")
        {
            auto out_frame = ws::frame_generator{}.close(
                    /*code=*/1000, /*reason=*/{}, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize + sizeof(status_code));

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Close);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == sizeof(status_code));
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            std::memcpy(reinterpret_cast<void*>(&status_code),
                    reinterpret_cast<void const*>(frame.get_payload_data().data()),
                    sizeof(status_code));
            status_code = std::byteswap(status_code);
            REQUIRE(status_code == 1000);
        }

        SECTION("unmasked, custom status code, no reason")
        {
            auto out_frame = ws::frame_generator{}.close(1234);
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize + sizeof(status_code));

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Close);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == sizeof(status_code));
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            std::uint16_t status_code;
            std::memcpy(reinterpret_cast<void*>(&status_code),
                    reinterpret_cast<void const*>(frame.get_payload_data().data()),
                    sizeof(status_code));
            status_code = std::byteswap(status_code);
            REQUIRE(status_code == 1234);
        }

        SECTION("masked, custom status code, no reason")
        {
            auto out_frame = ws::frame_generator{}.close(1234, /*reason=*/{}, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize + sizeof(status_code));

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Close);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == sizeof(status_code));
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            std::uint16_t status_code;
            std::memcpy(reinterpret_cast<void*>(&status_code),
                    reinterpret_cast<void const*>(frame.get_payload_data().data()),
                    sizeof(status_code));
            status_code = std::byteswap(status_code);
            REQUIRE(status_code == 1234);
        }

        SECTION("unmasked custom status code and reason")
        {
            std::string reason = "this is a test";
            auto out_frame = ws::frame_generator{}.close(5678, reason);
            REQUIRE(out_frame.data().size()
                    == UnmaskedHeaderSize + sizeof(status_code) + reason.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Close);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == reason.size() + sizeof(status_code));
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            // status code is in big endian
            std::memcpy(reinterpret_cast<void*>(&status_code),
                    reinterpret_cast<void const*>(frame.get_payload_data().data()),
                    sizeof(status_code));
            status_code = std::byteswap(status_code);
            REQUIRE(status_code == 5678);

            // ignore the 2 byte status code at the beginning of the
            // payload
            auto payload = frame.get_payload_data();
            std::string_view sv(reinterpret_cast<char const*>(payload.data() + sizeof(status_code)),
                    frame.payload_len() - sizeof(status_code));
            REQUIRE(sv == reason);
        }

        SECTION("masked custom status code and reason")
        {
            std::string reason = "test";
            auto out_frame = ws::frame_generator{}.close(5678, reason, /*masked=*/true);
            REQUIRE(out_frame.data().size()
                    == MaskedHeaderSize + sizeof(status_code) + reason.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Close);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == reason.size() + sizeof(status_code));
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            // status code is in big endian
            std::memcpy(reinterpret_cast<void*>(&status_code),
                    reinterpret_cast<void const*>(frame.get_payload_data().data()),
                    sizeof(status_code));
            status_code = std::byteswap(status_code);
            REQUIRE(status_code == 5678);

            // ignore the 2 byte status code at the beginning of the
            // payload
            auto payload = frame.get_payload_data();
            std::string_view sv(reinterpret_cast<char const*>(payload.data() + sizeof(status_code)),
                    frame.payload_len() - sizeof(status_code));
            REQUIRE(sv == reason);
        }
    }

    SECTION("text")
    {
        SECTION("unmasked empty payload with fin set")
        {
            auto out_frame = ws::frame_generator{}.text({});
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Text);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
            REQUIRE(frame.get_text_payload().has_value());
            REQUIRE(frame.get_text_payload().value().empty());
        }

        SECTION("unmasked empty payload without fin set")
        {
            auto out_frame = ws::frame_generator{}.text({}, /*fin=*/false);
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Text);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
            REQUIRE(frame.get_text_payload().has_value());
            REQUIRE(frame.get_text_payload().value().empty());
        }

        SECTION("masked empty payload with fin set")
        {
            auto out_frame = ws::frame_generator{}.text({}, /*fin=*/true, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Text);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == MaskedHeaderSize);
            REQUIRE(frame.get_text_payload().has_value());
            REQUIRE(frame.get_text_payload().value().empty());
        }

        SECTION("masked empty payload without fin set")
        {
            auto out_frame = ws::frame_generator{}.text({}, /*fin=*/false, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Text);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == MaskedHeaderSize);
            REQUIRE(frame.get_text_payload().has_value());
            REQUIRE(frame.get_text_payload().value().empty());
        }

        SECTION("unmasked with fin set")
        {
            std::string text = "hello";
            auto out_frame = ws::frame_generator{}.text(text);
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize + text.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Text);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == text.size());
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            REQUIRE(frame.get_text_payload().has_value());
            REQUIRE(frame.get_text_payload().value() == text);
        }

        SECTION("unmasked without fin")
        {
            std::string text = "hello";
            auto out_frame = ws::frame_generator{}.text(text, /*fin=*/false);
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize + text.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Text);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == text.size());
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            REQUIRE(frame.get_text_payload().has_value());
            REQUIRE(frame.get_text_payload().value() == text);
        }

        SECTION("masked with fin")
        {
            std::string text = "hello";
            auto out_frame = ws::frame_generator{}.text(text, /*fin=*/true, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize + text.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Text);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == text.size());
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            REQUIRE(frame.get_text_payload().has_value());
            REQUIRE(frame.get_text_payload().value() == text);
        }

        SECTION("masked without fin")
        {
            std::string text = "hello";
            auto out_frame = ws::frame_generator{}.text(text, /*fin=*/false, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize + text.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Text);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == text.size());
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            REQUIRE(frame.get_text_payload().has_value());
            REQUIRE(frame.get_text_payload().value() == text);
        }
    }

    SECTION("binary")
    {
        SECTION("unmasked empty payload with fin set")
        {
            auto out_frame = ws::frame_generator{}.binary({});
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Binary);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
            REQUIRE(frame.get_payload_data().empty());
        }

        SECTION("unmasked empty payload without fin set")
        {
            auto out_frame = ws::frame_generator{}.binary({}, /*fin=*/false);
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Binary);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
            REQUIRE(frame.get_payload_data().empty());
        }

        SECTION("masked empty payload with fin set")
        {
            auto out_frame = ws::frame_generator{}.binary({}, /*fin=*/true, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Binary);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == MaskedHeaderSize);
            REQUIRE(frame.get_payload_data().empty());
        }

        SECTION("masked empty payload without fin set")
        {
            auto out_frame = ws::frame_generator{}.binary({}, /*fin=*/false, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Binary);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == MaskedHeaderSize);
            REQUIRE(frame.get_payload_data().empty());
        }

        SECTION("default (with fin set)")
        {
            std::string data = "binary_data";
            auto out_frame = ws::frame_generator{}.binary(as_uint8_span(data));
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize + data.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Binary);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == data.size());
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            auto payload = frame.get_payload_data();
            REQUIRE(as_str(payload) == data);
        }

        SECTION("unmasked without fin")
        {
            std::string data = "binary_data";
            auto out_frame = ws::frame_generator{}.binary(
                    as_uint8_span(data), /*fin=*/false, /*masked=*/false);
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize + data.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Binary);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == data.size());
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            auto payload = frame.get_payload_data();
            REQUIRE(as_str(payload) == data);
        }

        SECTION("masked with fin")
        {
            std::string data = "binary_data";
            auto out_frame = ws::frame_generator{}.binary(
                    as_uint8_span(data), /*fin=*/true, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize + data.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Binary);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == data.size());
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            auto payload = frame.get_payload_data();
            REQUIRE(as_str(payload) == data);
        }

        SECTION("masked without fin")
        {
            std::string data = "binary_data";
            auto out_frame = ws::frame_generator{}.binary(
                    as_uint8_span(data), /*fin=*/false, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize + data.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Binary);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == data.size());
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            auto payload = frame.get_payload_data();
            REQUIRE(as_str(payload) == data);
        }
    }

    SECTION("continuation")
    {
        SECTION("unmasked empty payload with fin set")
        {
            auto out_frame = ws::frame_generator{}.continuation({}, /*fin=*/true);
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Continuation);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
            REQUIRE(frame.get_payload_data().empty());
        }

        SECTION("unmasked empty payload without fin set")
        {
            auto out_frame = ws::frame_generator{}.continuation({});
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Continuation);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);
            REQUIRE(frame.get_payload_data().empty());
        }

        SECTION("masked empty payload with fin set")
        {
            auto out_frame = ws::frame_generator{}.continuation({}, /*fin=*/true, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Continuation);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == MaskedHeaderSize);
            REQUIRE(frame.get_payload_data().empty());
        }

        SECTION("masked empty payload without fin set")
        {
            auto out_frame = ws::frame_generator{}.continuation({}, /*fin=*/false, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize);

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Continuation);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == 0);
            REQUIRE(frame.header_size() == MaskedHeaderSize);
            REQUIRE(frame.get_payload_data().empty());
        }

        SECTION("default (without fin set)")
        {
            std::string data = "binary_data";
            auto out_frame = ws::frame_generator{}.continuation(as_uint8_span(data));
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize + data.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Continuation);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == data.size());
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            auto payload = frame.get_payload_data();
            REQUIRE(as_str(payload) == data);
        }

        SECTION("unmasked without fin")
        {
            std::string data = "binary_data";
            auto out_frame = ws::frame_generator{}.continuation(
                    as_uint8_span(data), /*fin=*/false, /*masked=*/false);
            REQUIRE(out_frame.data().size() == UnmaskedHeaderSize + data.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Continuation);
            REQUIRE_FALSE(frame.masked());
            REQUIRE(frame.payload_len() == data.size());
            REQUIRE(frame.header_size() == UnmaskedHeaderSize);

            auto payload = frame.get_payload_data();
            REQUIRE(as_str(payload) == data);
        }

        SECTION("masked with fin")
        {
            std::string data = "binary_data";
            auto out_frame = ws::frame_generator{}.continuation(
                    as_uint8_span(data), /*fin=*/true, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize + data.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Continuation);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == data.size());
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            auto payload = frame.get_payload_data();
            REQUIRE(as_str(payload) == data);
        }

        SECTION("masked without fin")
        {
            std::string data = "binary_data";
            auto out_frame = ws::frame_generator{}.continuation(
                    as_uint8_span(data), /*fin=*/false, /*masked=*/true);
            REQUIRE(out_frame.data().size() == MaskedHeaderSize + data.size());

            ws::frame frame;
            REQUIRE(frame.parse_from_buffer(out_frame.data().data(), out_frame.data().size())
                    == ParseResult::Success);
            REQUIRE_FALSE(frame.fin());
            REQUIRE_FALSE(frame.rsv1());
            REQUIRE_FALSE(frame.rsv2());
            REQUIRE_FALSE(frame.rsv3());
            REQUIRE(frame.op_code() == OpCode::Continuation);
            REQUIRE(frame.masked());
            REQUIRE(frame.payload_len() == data.size());
            REQUIRE(frame.header_size() == MaskedHeaderSize);

            auto payload = frame.get_payload_data();
            REQUIRE(as_str(payload) == data);
        }
    }

    SECTION("fragmentation")
    {
        SECTION("fragmented text message - 3 parts")
        {
            std::string const full_text = "This message will be split into three fragments";
            std::string const part1 = "This message ";
            std::string const part2 = "will be split ";
            std::string const part3 = "into three fragments";

            // generate frames
            auto frame1 = ws::frame_generator{}.text(part1, /*fin=*/false, /*mask=*/true);
            auto frame2 = ws::frame_generator{}.continuation(
                    as_uint8_span(part2), /*fin=*/false, /*mask=*/true);
            auto frame3 = ws::frame_generator{}.continuation(
                    as_uint8_span(part3), /*fin=*/true, /*mask=*/true);

            // parse and verify each frame
            ws::frame parsed_frame1, parsed_frame2, parsed_frame3;

            REQUIRE(parsed_frame1.parse_from_buffer(frame1.data().data(), frame1.size())
                    == ParseResult::Success);
            REQUIRE(parsed_frame2.parse_from_buffer(frame2.data().data(), frame2.size())
                    == ParseResult::Success);
            REQUIRE(parsed_frame3.parse_from_buffer(frame3.data().data(), frame3.size())
                    == ParseResult::Success);

            // Verify frame properties
            REQUIRE_FALSE(parsed_frame1.fin());
            REQUIRE(parsed_frame1.op_code() == OpCode::Text);
            REQUIRE_FALSE(parsed_frame2.fin());
            REQUIRE(parsed_frame2.op_code() == OpCode::Continuation);
            REQUIRE(parsed_frame3.fin());
            REQUIRE(parsed_frame3.op_code() == OpCode::Continuation);
        }
    }
}

} // namespace ws::test
