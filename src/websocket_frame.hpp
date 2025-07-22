#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
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
    std::uint8_t byte1; // FIN, RSV1-3, OpCode
    std::uint8_t byte2; // MASK, payload length (7 bits)

    bool fin() const noexcept;
    bool rsv1() const noexcept;
    bool rsv2() const noexcept;
    bool rsv3() const noexcept;
    OpCode op_code() const noexcept;
    bool masked() const noexcept;
    std::uint8_t payload_len_indicator() const noexcept;
} __attribute__((packed));

/// parsed WebSocket frame information
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

public:
    websocket_frame() = default;

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

    /// Parse frame from buffer data
    /// \return ParseResult indicating success, need more data, or invalid frame
    ParseResult parse_from_buffer(std::uint8_t const*, std::size_t) noexcept;

    /// Get payload data from buffer (assumes frame was successfully parsed)
    /// \param buffer_start Start of the buffer used in parse_from_buffer
    /// \return Span pointing to payload data
    std::span<std::uint8_t const> get_payload_data(std::uint8_t const* buffer_start) const noexcept;

    /// Unmask payload data if frame is masked
    /// \param masked_payload The masked payload data
    /// \return Unmasked payload data
    std::vector<std::uint8_t> unmask_payload(std::span<std::uint8_t const> masked_payload) const;

    /// Get text payload as string (for text frames)
    /// \param buffer_start Start of the buffer used in parse_from_buffer
    /// \return String view of the text payload (unmasked if necessary)
    std::optional<std::string> get_text_payload(std::uint8_t const* buffer_start) const;

    /// Validate frame according to RFC 6455
    bool is_valid_frame() const noexcept;

    /// Reset frame to initial state
    void reset() noexcept;

private:
    /// read big-endian 16-bit value
    static std::uint16_t read_be16(std::uint8_t const*) noexcept;

    /// read big-endian 64-bit value
    static std::uint64_t read_be64(std::uint8_t const*) noexcept;
};

} // namespace ws
