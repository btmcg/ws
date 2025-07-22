#pragma once

#include "connection.hpp"
#include <format>
#include <ranges> // std::ranges::copy
#include <string_view>

namespace ws {

constexpr std::string_view
to_string(ConnectionState s) noexcept
{
    switch (s) {
        case ConnectionState::Http:
            return "Http";
        case ConnectionState::WebSocket:
            return "WebSocket";
        case ConnectionState::WebSocketClosing:
            return "WebSocketClosing";
        case ConnectionState::Undefined:
            return "Undefined";
    }
    return "???";
}

// ParseState
// constexpr std::string_view
// to_string(ParseState s) noexcept
// {
//     switch (s) {
//         case ParseState::ReadingHeader:
//             return "ReadingHeader";
//         case ParseState::ReadingExtendedLen16:
//             return "ReadingExtendedLen16";
//         case ParseState::ReadingExtendedLen64:
//             return "ReadingExtendedLen64";
//         case ParseState::ReadingMask:
//             return "ReadingMask";
//         case ParseState::ReadingPayload:
//             return "ReadingPayload";
//         case ParseState::Undefined:
//             return "Undefined";
//     }
//     return "???";
// }

} // namespace ws


// formatters must be in std

template <>
struct std::formatter<ws::ConnectionState>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(ws::ConnectionState s, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", ws::to_string(s));
    }
};


// template <>
// struct std::formatter<ws::ParseState>
// {
//     constexpr auto
//     parse(std::format_parse_context& ctx)
//     {
//         return ctx.begin();
//     }

//     auto
//     format(ws::ParseState s, std::format_context& ctx) const
//     {
//         return std::ranges::copy(ws::to_string(s), ctx.out()).out;
//     }
// };


// template <>
// struct std::formatter<ws::connection>
// {
//     constexpr auto
//     parse(std::format_parse_context& ctx)
//     {
//         return ctx.begin();
//     }

//     auto
//     format(ws::connection const& c, std::format_context& ctx) const
//     {
//         return std::format_to(ctx.out(),
//                 "connection(ip={},port={},conn_state={},parse_state={},bytes_needed={},payload_bytes_read={})",
//                 std::string_view(c.ip), c.port, c.conn_state, c.parse_state, c.bytes_needed,
//                 c.payload_bytes_read);
//     }
// };
