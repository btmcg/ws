#pragma once

#include <cstdint>
#include <vector>

namespace ws {

enum class op_code : std::uint8_t
{
    Continuation = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xa
};

struct websocket_frame
{
    bool fin;
    op_code op;
    std::vector<std::uint8_t> payload;
};

} // namespace ws
