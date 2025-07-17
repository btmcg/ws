#include "sha1.hpp"
#include <cstdlib> // std::memcpy
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

namespace ws {

constexpr std::uint32_t
sha1::left_rotate(std::uint32_t value, std::size_t amount)
{
    return (value << amount) | (value >> (32 - amount));
}

sha1::digest_type
sha1::hash(std::string_view input)
{
    return hash(reinterpret_cast<const std::uint8_t*>(input.data()), input.size());
}

sha1::digest_type
sha1::hash(const std::uint8_t* data, std::size_t length)
{
    // Initialize hash values (SHA-1 initial hash values)
    std::array<std::uint32_t, 5> hash_values
            = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

    // Pre-processing: adding padding bits
    std::uint64_t const original_bit_len = length * 8;
    std::uint64_t const original_byte_len = length;

    // Calculate padded length
    std::uint64_t padded_len = original_byte_len + 1; // +1 for the '1' bit (0x80 byte)
    while (padded_len % 64 != 56) {                   // Leave 8 bytes for length
        padded_len++;
    }
    padded_len += 8; // Add 8 bytes for the original length

    // Create padded message
    std::vector<std::uint8_t> padded_message(padded_len);

    // Copy original data
    std::memcpy(padded_message.data(), data, original_byte_len);

    // Add padding
    padded_message[original_byte_len] = 0x80; // Single '1' bit followed by zeros

    // Fill remaining padding with zeros (already initialized to 0)

    // Append original length as 64-bit big-endian integer
    for (int i = 0; i < 8; ++i) {
        padded_message[padded_len - 8 + i]
                = static_cast<std::uint8_t>((original_bit_len >> (8 * (7 - i))) & 0xFF);
    }

    // Process message in 512-bit chunks
    for (std::size_t chunk_start = 0; chunk_start < padded_len; chunk_start += 64) {
        process_chunk(hash_values, &padded_message[chunk_start]);
    }

    // Convert hash values to byte array (big-endian)
    digest_type result;
    for (std::size_t i = 0; i < 5; ++i) {
        std::uint32_t const h = hash_values[i];
        result[i * 4 + 0] = static_cast<std::uint8_t>((h >> 24) & 0xFF);
        result[i * 4 + 1] = static_cast<std::uint8_t>((h >> 16) & 0xFF);
        result[i * 4 + 2] = static_cast<std::uint8_t>((h >> 8) & 0xFF);
        result[i * 4 + 3] = static_cast<std::uint8_t>(h & 0xFF);
    }

    return result;
}

void
sha1::process_chunk(std::array<std::uint32_t, 5>& hash, const std::uint8_t* chunk)
{
    // Break chunk into sixteen 32-bit big-endian words
    std::array<std::uint32_t, 80> w{};

    for (std::size_t i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(chunk[i * 4 + 0]) << 24)
                | (static_cast<std::uint32_t>(chunk[i * 4 + 1]) << 16)
                | (static_cast<std::uint32_t>(chunk[i * 4 + 2]) << 8)
                | static_cast<std::uint32_t>(chunk[i * 4 + 3]);
    }

    // Extend the sixteen 32-bit words into eighty 32-bit words
    for (std::size_t i = 16; i < 80; ++i) {
        w[i] = left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    // Initialize hash value for this chunk
    std::uint32_t a = hash[0];
    std::uint32_t b = hash[1];
    std::uint32_t c = hash[2];
    std::uint32_t d = hash[3];
    std::uint32_t e = hash[4];

    // Main loop
    for (std::size_t i = 0; i < 80; ++i) {
        std::uint32_t f, k;

        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        std::uint32_t const temp = left_rotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = left_rotate(b, 30);
        b = a;
        a = temp;
    }

    // Add this chunk's hash to result so far
    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
}

std::string
sha1::hash_hex(std::string_view input)
{
    auto const digest = hash(input);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (std::uint8_t const byte : digest) {
        oss << std::setw(2) << static_cast<unsigned>(byte);
    }

    return oss.str();
}

// Convenience functions
sha1::digest_type
sha1_hash(std::string_view input)
{
    return sha1::hash(input);
}

std::string
sha1_hash_hex(std::string_view input)
{
    return sha1::hash_hex(input);
}

} // namespace ws
