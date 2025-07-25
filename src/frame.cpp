#include "frame.hpp"
#include <bit>     // std::byteswap, std::endian
#include <cstring> // std::memcpy, std::memset

namespace ws {

bool
basic_websocket_header::fin() const noexcept
{
    return byte1 & 0b1000'0000;
}

bool
basic_websocket_header::rsv1() const noexcept
{
    return byte1 & 0b0100'0000;
}

bool
basic_websocket_header::rsv2() const noexcept
{
    return byte1 & 0b0010'0000;
}

bool
basic_websocket_header::rsv3() const noexcept
{
    return byte1 & 0b0001'0000;
}

OpCode
basic_websocket_header::op_code() const noexcept
{
    return static_cast<OpCode>(byte1 & 0b0000'1111);
}

bool
basic_websocket_header::masked() const noexcept
{
    return byte2 & 0b1000'0000;
}

std::uint8_t
basic_websocket_header::payload_len_indicator() const noexcept
{
    return byte2 & 0b0111'1111;
}

/**********************************************************************/

void
frame::reset() noexcept
{
    fin_ = false;
    rsv1_ = rsv2_ = rsv3_ = false;
    op_code_ = OpCode::Continuation;
    masked_ = false;
    payload_len_ = 0;
    std::memset(masking_key_, 0, sizeof(masking_key_));
    header_size_ = 0;
    valid_ = false;
    payload_data_.clear();
}

ParseResult
frame::parse_from_buffer(std::uint8_t const* data, std::size_t avail) noexcept
{
    reset();

    if (avail < MinFrameHeaderSize) {
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
        if (avail < header_size_ + 2) {
            return ParseResult::NeedMoreData;
        }
        payload_len_ = read_be16(data + header_size_);
        header_size_ += 2;

        // payload length < 126 should not use extended format
        if (payload_len_ < 126) {
            return ParseResult::InvalidFrame;
        }
    } else { // payload_indicator == 127
        if (avail < header_size_ + 8) {
            return ParseResult::NeedMoreData;
        }
        payload_len_ = read_be64(data + header_size_);
        header_size_ += 8;

        // payload length < 65536 should not use 64-bit format
        if (payload_len_ < 65536) {
            return ParseResult::InvalidFrame;
        }

        // MSB must be 0 (no payloads > 2^63-1)
        if (payload_len_ & 0x8000000000000000ull) {
            return ParseResult::InvalidFrame;
        }
    }

    // parse masking key if present
    if (masked_) {
        if (avail < header_size_ + 4) {
            return ParseResult::NeedMoreData;
        }
        std::memcpy(masking_key_, data + header_size_, 4);
        header_size_ += 4;
    }

    // check if we have complete frame
    if (avail < header_size_ + payload_len_) {
        return ParseResult::NeedMoreData;
    }

    // extract and store payload data
    std::uint8_t const* payload_start = data + header_size_;
    payload_data_.resize(payload_len_);

    if (masked_) {
        // store unmasked payload
        for (std::uint64_t i = 0; i < payload_len_; ++i) {
            payload_data_[i] = payload_start[i] ^ masking_key_[i % 4];
        }
    } else {
        // store payload directly
        std::memcpy(payload_data_.data(), payload_start, payload_len_);
    }

    // additional validation
    if (!is_valid_frame()) {
        return ParseResult::InvalidFrame;
    }

    valid_ = true;
    return ParseResult::Success;
}

bool
frame::fin() const noexcept
{
    return fin_;
}

bool
frame::rsv1() const noexcept
{
    return rsv1_;
}

bool
frame::rsv2() const noexcept
{
    return rsv2_;
}

bool
frame::rsv3() const noexcept
{
    return rsv3_;
}

OpCode
frame::op_code() const noexcept
{
    return op_code_;
}

bool
frame::masked() const noexcept
{
    return masked_;
}

std::uint64_t
frame::payload_len() const noexcept
{
    return payload_len_;
}

std::size_t
frame::header_size() const noexcept
{
    return header_size_;
}

bool
frame::valid() const noexcept
{
    return valid_;
}

std::span<std::uint8_t const, 4>
frame::masking_key() const noexcept
{
    return std::span<std::uint8_t const, 4>(masking_key_, 4);
}

std::uint64_t
frame::total_size() const noexcept
{
    return header_size_ + payload_len_;
}


std::span<std::uint8_t const>
frame::get_payload_data() const noexcept
{
    return std::span<std::uint8_t const>(payload_data_);
}

std::span<std::uint8_t const>
frame::get_raw_payload_data() const noexcept
{
    // This would require storing the original masked data separately. For now, we only store the
    // unmasked version
    return get_payload_data();
}

std::optional<std::string>
frame::get_text_payload() const
{
    if (!valid_ || op_code_ != OpCode::Text) {
        return std::nullopt;
    }

    // payload_data_ is already unmasked if it was masked
    return std::string(reinterpret_cast<char const*>(payload_data_.data()), payload_data_.size());
}

bool
frame::is_valid_frame() const noexcept
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
    if (opcode_val >= 0xb && opcode_val <= 0xf) {
        return false; // reserved for future control frames
    }

    // client-to-server frames must be masked
    return true;
}

std::uint16_t
frame::read_be16(const std::uint8_t* data) noexcept
{
    std::uint16_t value;
    std::memcpy(&value, data, sizeof(value));
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(value);
    }
    return value;
}

std::uint64_t
frame::read_be64(const std::uint8_t* data) noexcept
{
    std::uint64_t value;
    std::memcpy(&value, data, sizeof(value));
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(value);
    }
    return value;
}

} // namespace ws
