#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ws {

/// min number of bytes needed to read a frame's basic header
static constexpr std::size_t MinFrameHeaderSize = 2;

/// max number of bytes in a frame header (2 basic + 8 extended + 4 mask)
static constexpr std::size_t MaxFrameHeaderSize = 14;

enum class OpCode : std::uint8_t
{
    Continuation = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xa
};

enum class ParseResult
{
    Success,
    NeedMoreData,
    InvalidFrame
};

/// Represents the basic 2-byte WebSocket frame header
struct basic_websocket_header
{
    std::uint8_t byte1 = 0; // FIN, RSV1-3, OpCode
    std::uint8_t byte2 = 0; // MASK, payload length (7 bits)

    bool fin() const noexcept;
    bool rsv1() const noexcept;
    bool rsv2() const noexcept;
    bool rsv3() const noexcept;
    OpCode op_code() const noexcept;
    bool masked() const noexcept;
    std::uint8_t payload_len_indicator() const noexcept;
} __attribute__((packed));

/// Parsed WebSocket frame information
class websocket_frame
{
private:
    bool fin_ = false;
    bool rsv1_ = false;
    bool rsv2_ = false;
    bool rsv3_ = false;
    OpCode op_code_ = OpCode::Continuation;
    bool masked_ = false;
    std::uint64_t payload_len_ = 0;
    std::uint8_t masking_key_[4] = {0};
    std::size_t header_size_ = 0;
    bool valid_ = false;
    std::vector<std::uint8_t> payload_data_; // store the actual payload data

public:
    websocket_frame() = default;

    /// reset frame to initial state
    void reset() noexcept;

public:
    /// Parse frame from buffer data
    /// \return ParseResult indicating success, need more data, or invalid frame
    ParseResult parse_from_buffer(std::uint8_t const*, std::size_t) noexcept;

public:
    bool fin() const noexcept;
    bool rsv1() const noexcept;
    bool rsv2() const noexcept;
    bool rsv3() const noexcept;
    OpCode op_code() const noexcept;
    bool masked() const noexcept;
    std::uint64_t payload_len() const noexcept;
    std::size_t header_size() const noexcept;
    bool valid() const noexcept;
    std::span<std::uint8_t const, 4> masking_key() const noexcept;

    /// total frame size (header + payload)
    std::uint64_t total_size() const noexcept;

    /// Get payload data (automatically unmasked if necessary)
    /// @return Span pointing to payload data
    std::span<std::uint8_t const> get_payload_data() const noexcept;

    /// Get raw payload data (still masked if frame was masked)
    /// @return Span pointing to raw payload data
    std::span<std::uint8_t const> get_raw_payload_data() const noexcept;

    /// Get text payload as string (for text frames)
    /// @return String of the text payload (automatically unmasked)
    std::optional<std::string> get_text_payload() const;

private:
    /// validate frame according to RFC 6455
    bool is_valid_frame() const noexcept;

    /// read big-endian 16-bit value
    static std::uint16_t read_be16(std::uint8_t const*) noexcept;

    /// read big-endian 64-bit value
    static std::uint64_t read_be64(std::uint8_t const*) noexcept;
};

} // namespace ws
