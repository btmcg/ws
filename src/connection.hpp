#pragma once

#include "websocket_frame.hpp"
#include <cstdint>
#include <format>
#include <vector>

namespace ws {

enum class ConnectionState : std::uint8_t
{
    Http,
    Websocket,
    WebsocketClosing,
    Undefined
};

template <>
struct std::formatter<ConnectionState>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(ConnectionState s, std::format_context& ctx) const
    {
        switch (s) {
            case Http:
                return std::format_to(ctx.out(), "Http");
            case Websocket:
                return std::format_to(ctx.out(), "Websocket");
            case WebsocketClosing:
                return std::format_to(ctx.out(), "WebsocketClosing");
            case Undefined:
                return std::format_to(ctx.out(), "Undefined");
            default:
                return std::format_to(ctx.out(), "???");
        }
    }
};

enum class ParseState : std::uint8_t
{
    ReadingHeader,
    ReadingExtendedLen16,
    ReadingExtendedLen64,
    ReadingMask,
    ReadingPayload,
    Undefined
};

template <>
struct std::formatter<ParseState>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(ParseState s, std::format_context& ctx) const
    {
        switch (s) {
            case ReadingHeader:
                return std::format_to(ctx.out(), "ReadingHeader");
            case ReadingExtendedLen16:
                return std::format_to(ctx.out(), "ReadingExtendedLen16");
            case ReadingExtendedLen64:
                return std::format_to(ctx.out(), "ReadingExtendedLen64");
            case ReadingMask:
                return std::format_to(ctx.out(), "ReadingMask");
            case ReadingPayload:
                return std::format_to(ctx.out(), "ReadingPayload");
            case Undefined:
                return std::format_to(ctx.out(), "Undefined");
            default:
                return std::format_to(ctx.out(), "???");
        }
    }
};

struct connection
{
    std::vector<std::uint8_t> buf;
    ConnectionState conn_state = ConnectionState::Http;
    ParseState parse_state = ParseState::ReadingHeader;
    websocket_frame current_frame;
    std::uint64_t bytes_needed = 2; // start with basic header
    std::uint64_t payload_bytes_read = 0;
};

// Specialize std::formatter for your struct
template <>
struct std::formatter<connection>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin(); // Simple case: no custom format specifiers
    }

    auto
    format(connection const& p, std::format_context& ctx) const
    {
        return std::format_to(
                ctx.out(), "connection(conn_state={}Person{{name: {}, age: {}}}", p.name, p.age);
    }
};


} // namespace ws
