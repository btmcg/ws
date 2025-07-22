#pragma once

#include <cstdint>
#include <vector>


namespace ws {
/// min number of bytes needed to read a frame's payload length
static constexpr int MinFrameBytesNeeded = 2;

enum class OpCode : std::uint8_t
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
    std::uint8_t byte1; //  fin, rsv1, rsv2, rsv3, op_code
    std::uint8_t byte2; //  masked, payload_len (7 bits)

    std::uint16_t extended_payload_len_16{}; ///< used when payload_len == 126
    std::uint64_t extended_payload_len_64{}; ///< used when payload_len == 127
    std::uint8_t masking_key[4]{};

    bool
    fin() const noexcept
    {
        return byte1 & 0b1000'0000;
    }

    bool
    rsv1() const noexcept
    {
        return byte1 & 0b0100'0000;
    }

    bool
    rsv2() const noexcept
    {
        return byte1 & 0b0010'0000;
    }

    bool
    rsv3() const noexcept
    {
        return byte1 & 0b0001'0000;
    }

    OpCode
    op_code() const noexcept
    {
        return static_cast<OpCode>(byte1 & 0b0000'1111);
    }

    bool
    masked() const noexcept
    {
        return byte2 & 0b1000'0000;
    }

    std::uint64_t
    payload_len() const noexcept
    {
        std::uint8_t len = byte2 & 0b0111'1111;
        if (len < 126) {
            return len;
        }
        if (len == 126) {
            return std::byteswap(extended_payload_len_16);
        }
        return std::byteswap(extended_payload_len_64);
    }
} __attribute__((packed));

} // namespace ws
