#pragma once

#include "frame.hpp"
#include <span>
#include <string_view>
#include <vector>

namespace ws {

/*! \class  frame_generator
 *  \brief  This is a generator that can be used to parse data into a
 *          websocket frame or to generate a frame in preparation to
 *          send.
 */
class frame_generator
{
private:
    std::vector<std::uint8_t> frame_data_;

public:
    frame_generator() = default;

    /// Create a ping frame
    /// \param payload Optional payload (max 125 bytes)
    /// \param mask Whether to mask the payload (default: false for server-to-client)
    /// \return Reference to this generator for method chaining
    frame_generator& ping(std::span<std::uint8_t const> payload = {}, bool mask = false);

    /// Create a pong frame
    /// \param payload Optional payload (max 125 bytes, usually echoes ping payload)
    /// \param mask Whether to mask the payload (default: false for server-to-client)
    /// \return Reference to this generator for method chaining
    frame_generator& pong(std::span<std::uint8_t const> payload = {}, bool mask = false);

    /// Create a close frame
    /// \param code Optional close code (default: 1000 = normal closure)
    /// \param reason Optional close reason (combined with code, max 125 bytes total)
    /// \param mask Whether to mask the payload (default: false for server-to-client)
    /// \return Reference to this generator for method chaining
    frame_generator& close(
            std::uint16_t code = 1000, std::string_view reason = "", bool mask = false);

    /// Create a text frame
    /// \param text The text payload
    /// \param fin Whether this is the final fragment (default: true)
    /// \param mask Whether to mask the payload (default: false for server-to-client)
    /// \return Reference to this generator for method chaining
    frame_generator& text(std::string_view text, bool fin = true, bool mask = false);

    /// Create a binary frame
    /// \param data The binary payload
    /// \param fin Whether this is the final fragment (default: true)
    /// \param mask Whether to mask the payload (default: false for server-to-client)
    /// \return Reference to this generator for method chaining
    frame_generator& binary(std::span<std::uint8_t const> data, bool fin = true, bool mask = false);

    /// Create a continuation frame
    /// \param data The payload data
    /// \param fin Whether this is the final fragment
    /// \param mask Whether to mask the payload (default: false for server-to-client)
    /// \return Reference to this generator for method chaining
    frame_generator& continuation(
            std::span<std::uint8_t const> data, bool fin = false, bool mask = false);

    /// Get the generated frame data ready for transmission
    /// \return Span of the complete frame bytes
    std::span<std::uint8_t const> data() const noexcept;

    /// Get the size of the generated frame
    /// \return Size in bytes
    std::size_t size() const noexcept;

    /// Clear the generator to build a new frame
    /// \return Reference to this generator for method chaining
    frame_generator& reset() noexcept;

    /// Move the frame data out (for zero-copy scenarios)
    /// \return The frame data vector
    std::vector<std::uint8_t> take_data() noexcept;

    static std::string generate_websocket_key() noexcept;

private:
    /// Build frame with given parameters
    void build_frame(OpCode opcode, std::span<std::uint8_t const> payload, bool fin, bool mask);

    /// Write big-endian 16-bit value
    static void write_be16(std::uint8_t* data, std::uint16_t value) noexcept;

    /// Write big-endian 64-bit value
    static void write_be64(std::uint8_t* data, std::uint64_t value) noexcept;

    /// Generate random masking key
    static std::array<std::uint8_t, 4> generate_mask() noexcept;

    /// Apply masking to payload
    static void apply_mask(std::uint8_t* payload, std::size_t length,
            std::array<std::uint8_t, 4> const& mask) noexcept;
};

} // namespace ws
