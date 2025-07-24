#pragma once

#include "websocket_frame.hpp"
#include <format>
#include <ranges> // std::ranges::copy
#include <string_view>

namespace ws {

constexpr std::string_view
to_string(OpCode o) noexcept
{
    switch (o) {
        case OpCode::Continuation:
            return "Continuation";
        case OpCode::Text:
            return "Text";
        case OpCode::Binary:
            return "Binary";
        case OpCode::Close:
            return "Close";
        case OpCode::Ping:
            return "Ping";
        case OpCode::Pong:
            return "Pong";
        default:
            return "???";
    }
    return "???";
}

} // namespace ws


// formatters must be in std

template <>
struct std::formatter<ws::OpCode>
{
    constexpr auto
    parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto
    format(ws::OpCode o, std::format_context& ctx) const
    {
        return std::ranges::copy(ws::to_string(o), ctx.out()).out;
    }
};
