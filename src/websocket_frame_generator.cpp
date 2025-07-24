#include "websocket_frame_generator.hpp"
#include <bit>     // std::byteswap, std::endian
#include <cstring> // std::memcpy
#include <random>  // std::mt19937, std::random_device,
                   // std::uniform_int_distribution
#include <stdexcept>

namespace ws {

websocket_frame_generator&
websocket_frame_generator::text(std::string_view text, bool fin, bool mask)
{
    std::span<std::uint8_t const> payload(
            reinterpret_cast<std::uint8_t const*>(text.data()), text.size());
    build_frame(OpCode::Text, payload, fin, mask);
    return *this;
}

websocket_frame_generator&
websocket_frame_generator::binary(std::span<std::uint8_t const> data, bool fin, bool mask)
{
    build_frame(OpCode::Binary, data, fin, mask);
    return *this;
}

websocket_frame_generator&
websocket_frame_generator::ping(std::span<std::uint8_t const> payload, bool mask)
{
    if (payload.size() > 125) {
        throw std::invalid_argument("Ping payload cannot exceed 125 bytes");
    }
    build_frame(OpCode::Ping, payload, true, mask);
    return *this;
}

websocket_frame_generator&
websocket_frame_generator::pong(std::span<std::uint8_t const> payload, bool mask)
{
    if (payload.size() > 125) {
        throw std::invalid_argument("Pong payload cannot exceed 125 bytes");
    }
    build_frame(OpCode::Pong, payload, true, mask);
    return *this;
}

websocket_frame_generator&
websocket_frame_generator::close(std::uint16_t code, std::string_view reason, bool mask)
{
    std::vector<std::uint8_t> close_payload;

    // add close code (big-endian)
    close_payload.resize(2);
    write_be16(close_payload.data(), code);

    // add reason if provided
    if (!reason.empty()) {
        if (reason.size() + 2 > 125) {
            throw std::invalid_argument("close payload (code + reason) cannot exceed 125 bytes");
        }
        close_payload.insert(close_payload.end(), reason.begin(), reason.end());
    }

    build_frame(OpCode::Close, std::span<std::uint8_t const>(close_payload), true, mask);
    return *this;
}

websocket_frame_generator&
websocket_frame_generator::continuation(std::span<std::uint8_t const> data, bool fin, bool mask)
{
    build_frame(OpCode::Continuation, data, fin, mask);
    return *this;
}

std::span<std::uint8_t const>
websocket_frame_generator::data() const noexcept
{
    return std::span<std::uint8_t const>(frame_data_);
}

std::size_t
websocket_frame_generator::size() const noexcept
{
    return frame_data_.size();
}

websocket_frame_generator&
websocket_frame_generator::reset() noexcept
{
    frame_data_.clear();
    return *this;
}

std::vector<std::uint8_t>
websocket_frame_generator::take_data() noexcept
{
    return std::move(frame_data_);
}

void
websocket_frame_generator::build_frame(
        OpCode opcode, std::span<std::uint8_t const> payload, bool fin, bool mask)
{
    frame_data_.clear();

    std::uint64_t const payload_len = payload.size();
    std::size_t header_size = 2; // Basic header

    // calculate extended length size
    if (payload_len >= 65536) {
        header_size += 8;
    } else if (payload_len >= 126) {
        header_size += 2;
    }

    // add masking key size if needed
    if (mask) {
        header_size += 4;
    }

    // reserve space for header + payload
    frame_data_.reserve(header_size + payload_len);
    frame_data_.resize(header_size);

    std::uint8_t* header = frame_data_.data();
    std::size_t header_pos = 0;

    // byte 1: FIN + RSV + OpCode
    header[header_pos++] = (fin ? 0x80 : 0x00) | static_cast<std::uint8_t>(opcode);

    // byte 2: MASK + Payload length
    std::uint8_t mask_bit = mask ? 0x80 : 0x00;

    if (payload_len < 126) {
        header[header_pos++] = mask_bit | static_cast<std::uint8_t>(payload_len);
    } else if (payload_len < 65536) {
        header[header_pos++] = mask_bit | 126;
        write_be16(&header[header_pos], static_cast<std::uint16_t>(payload_len));
        header_pos += 2;
    } else {
        header[header_pos++] = mask_bit | 127;
        write_be64(&header[header_pos], payload_len);
        header_pos += 8;
    }

    // add masking key if needed
    std::array<std::uint8_t, 4> masking_key{};
    if (mask) {
        masking_key = generate_mask();
        std::memcpy(&header[header_pos], masking_key.data(), 4);
        header_pos += 4;
    }

    // add payload
    if (!payload.empty()) {
        frame_data_.insert(frame_data_.end(), payload.begin(), payload.end());

        // Apply masking if needed
        if (mask) {
            apply_mask(&frame_data_[header_size], payload_len, masking_key);
        }
    }
}

void
websocket_frame_generator::write_be16(std::uint8_t* data, std::uint16_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::little) {
        value = std::byteswap(value);
    }
    std::memcpy(data, &value, sizeof(value));
}

void
websocket_frame_generator::write_be64(std::uint8_t* data, std::uint64_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::little) {
        value = std::byteswap(value);
    }
    std::memcpy(data, &value, sizeof(value));
}

std::array<std::uint8_t, 4>
websocket_frame_generator::generate_mask() noexcept
{
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_int_distribution<std::uint8_t> dis(0, 255);

    return {dis(gen), dis(gen), dis(gen), dis(gen)};
}

void
websocket_frame_generator::apply_mask(
        std::uint8_t* payload, std::size_t length, std::array<std::uint8_t, 4> const& mask) noexcept
{
    for (std::size_t i = 0; i < length; ++i) {
        payload[i] ^= mask[i % 4];
    }
}

} // namespace ws
