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
        case ConnectionState::TcpConnected:
            return "TcpConnected";
        case ConnectionState::Http:
            return "Http";
        case ConnectionState::WebSocket:
            return "WebSocket";
        case ConnectionState::WebSocketClosing:
            return "WebSocketClosing";
        case ConnectionState::Undefined:
            return "Undefined";
        default:
            return "???";
    }
    return "???";
}

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
        return std::ranges::copy(ws::to_string(s), ctx.out()).out;
    }
};


template <>
struct std::formatter<ws::connection>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(ws::connection const& c, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "connection(sockfd={},ip={},port={},conn_state={})",
                c.sockfd, std::string_view(c.ip), c.port, c.conn_state);
    }
};
