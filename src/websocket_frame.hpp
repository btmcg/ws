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
    bool fin : 1;
    std::uint8_t rsv1 : 1;
    std::uint8_t rsv2 : 1;
    std::uint8_t rsv3 : 1;
    op_code op : 4;

    std::uint8_t mask : 1;
    std::uint8_t payload_len : 7;

    std::uint16_t extended_payload_len_16; ///< used when payload_len == 126
    std::uint64_t extended_payload_len_64; ///< used when payload_len == 127

    std::uint8_t masking_key[4];

    std::uint64_t
    get_payload_len() const noexcept
    {
        if (payload_len < 126) {
            return payload_len;
        }
        if (payload_len == 126) {
            return std::byteswap(extended_payload_len_16);
        }
        return std::byteswap(extended_payload_len_64);
    }
} __attribute__((packed));

} // namespace ws
