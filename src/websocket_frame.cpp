#include "websocket_frame.hpp"

namespace ws {

bool
basic_websocket_header::fin() const noexcept
{
    return byte1 & 0b1000'0000;
    // return byte1 & 0x80;
}

bool
basic_websocket_header::rsv1() const noexcept
{
    return byte1 & 0b0100'0000;
    // return byte1 & 0x40;
}

bool
basic_websocket_header::rsv2() const noexcept
{
    return byte1 & 0b0010'0000;
    // return byte1 & 0x20;
}

bool
basic_websocket_header::rsv3() const noexcept
{
    return byte1 & 0b0001'0000;
    // return byte1 & 0x10;
}

OpCode
basic_websocket_header::op_code() const noexcept
{
    return static_cast<OpCode>(byte1 & 0b0000'1111);
    // return static_cast<OpCode>(byte1 & 0x0F);
}

bool
basic_websocket_header::masked() const noexcept
{
    return byte2 & 0b1000'0000;
    // return byte2 & 0x80;
}

std::uint8_t
basic_websocket_header::payload_len_indicator() const noexcept
{
    return byte2 & 0b0111'1111;
    // return byte2 & 0x7F;
}

/**********************************************************************/

bool
websocket_frame::fin() const noexcept
{
    return fin_;
}

bool
websocket_frame::rsv1() const noexcept
{
    return rsv1_;
}

bool
websocket_frame::rsv2() const noexcept
{
    return rsv2_;
}

bool
websocket_frame::rsv3() const noexcept
{
    return rsv3_;
}

OpCode
websocket_frame::op_code() const noexcept
{
    return op_code_;
}

bool
websocket_frame::masked() const noexcept
{
    return masked_;
}

std::uint64_t
websocket_frame::payload_len() const noexcept
{
    return payload_len_;
}

std::size_t
websocket_frame::header_size() const noexcept
{
    return header_size_;
}

bool
websocket_frame::valid() const noexcept
{
    return valid_;
}

std::span<const std::uint8_t, 4>
websocket_frame::masking_key() const noexcept
{
    return std::span<const std::uint8_t, 4>(masking_key_, 4);
}

std::uint64_t
websocket_frame::total_size() const noexcept
{
    return header_size_ + payload_len_;
}

ParseResult
websocket_frame::parse_from_buffer(std::uint8_t const* data, std::size_t available) noexcept
{
    reset();

    if (available < MinFrameHeaderSize) {
        return ParseResult::NeedMoreData;
    }

    // parse basic header
    auto const* header = reinterpret_cast<basic_websocket_header const*>(data);

    fin_ = header->fin();
    rsv1_ = header->rsv1();
    rsv2_ = header->rsv2();
    rsv3_ = header->rsv3();
    op_code_ = header->op_code();
    masked_ = header->masked();

    std::uint8_t payload_indicator = header->payload_len_indicator();
    header_size_ = MinFrameHeaderSize;

    // parse extended payload length
    if (payload_indicator < 126) {
        payload_len_ = payload_indicator;
    } else if (payload_indicator == 126) {
        if (available < header_size_ + 2) {
            return ParseResult::NeedMoreData;
        }
        payload_len_ = read_be16(data + header_size_);
        header_size_ += 2;

        // Validate: payload length < 126 should not use extended format
        if (payload_len_ < 126) {
            return ParseResult::InvalidFrame;
        }
    } else { // payload_indicator == 127
        if (available < header_size_ + 8) {
            return ParseResult::NeedMoreData;
        }
        payload_len_ = read_be64(data + header_size_);
        header_size_ += 8;

        // validate: payload length < 65536 should not use 64-bit format
        if (payload_len_ < 65536) {
            return ParseResult::InvalidFrame;
        }

        // validate: most significant byte must be 0 (no payloads > 2^63-1)
        if (payload_len_ & 0x8000000000000000ull) {
            return ParseResult::InvalidFrame;
        }
    }

    // parse masking key if present
    if (masked_) {
        if (available < header_size_ + 4) {
            return ParseResult::NeedMoreData;
        }
        std::memcpy(masking_key_, data + header_size_, 4);
        header_size_ += 4;
    }

    // check if we have complete frame
    if (available < header_size_ + payload_len_) {
        return ParseResult::NeedMoreData;
    }

    // additional validation
    if (!is_valid_frame()) {
        return ParseResult::InvalidFrame;
    }

    valid_ = true;
    return ParseResult::Success;
}

std::span<std::uint8_t const>
websocket_frame::get_payload_data(std::uint8_t const* buffer_start) const noexcept
{
    return std::span<const std::uint8_t>(buffer_start + header_size_, payload_len_);
}

std::vector<std::uint8_t>
websocket_frame::unmask_payload(std::span<std::uint8_t const> masked_payload) const
{
    std::vector<std::uint8_t> unmasked;
    unmasked.reserve(masked_payload.size());

    for (std::size_t i = 0; i < masked_payload.size(); ++i) {
        unmasked.push_back(masked_payload[i] ^ masking_key_[i % 4]);
    }

    return unmasked;
}

std::optional<std::string>
websocket_frame::get_text_payload(std::uint8_t const* buffer_start) const
{
    if (!valid_ || op_code_ != OpCode::Text) {
        return std::nullopt;
    }

    auto payload_data = get_payload_data(buffer_start);

    if (masked_) {
        auto unmasked = unmask_payload(payload_data);
        return std::string(reinterpret_cast<const char*>(unmasked.data()), unmasked.size());
    } else {
        return std::string(reinterpret_cast<const char*>(payload_data.data()), payload_data.size());
    }
}

bool
websocket_frame::is_valid_frame() const noexcept
{
    // check reserved bits (RSV1-3 must be 0 unless extensions are negotiated)
    if (rsv1_ || rsv2_ || rsv3_) {
        return false;
    }

    // validate opcode
    std::uint8_t opcode_val = static_cast<std::uint8_t>(op_code_);
    bool is_control = (opcode_val & 0x08) != 0;

    // control frames must have FIN=1
    if (is_control && !fin_) {
        return false;
    }

    // control frames must have payload <= 125 bytes
    if (is_control && payload_len_ > 125) {
        return false;
    }

    // reserved opcodes
    if (opcode_val >= 3 && opcode_val <= 7) {
        return false; // reserved for future non-control frames
    }
    if (opcode_val >= 0xB && opcode_val <= 0xF) {
        return false; // reserved for future control frames
    }
    return true;
}

void
websocket_frame::reset() noexcept
{
    fin_ = false;
    rsv1_ = rsv2_ = rsv3_ = false;
    op_code_ = OpCode::Continuation;
    masked_ = false;
    payload_len_ = 0;
    std::memset(masking_key_, 0, sizeof(masking_key_));
    header_size_ = 0;
    valid_ = false;
}

std::uint16_t
websocket_frame::read_be16(std::uint8_t const* data) noexcept
{
    std::uint16_t value;
    std::memcpy(&value, data, sizeof(value));
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(value);
    }
    return value;
}

std::uint64_t
websocket_frame::read_be64(std::uint8_t const* data) noexcept
{
    std::uint64_t value;
    std::memcpy(&value, data, sizeof(value));
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(value);
    }
    return value;
}

} // namespace ws
