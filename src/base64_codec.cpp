#include "base64_codec.hpp"

namespace ws {

// convenience functions
std::string
to_base64(std::string_view const input)
{
    return base64_codec::encode(input);
}

std::string
from_base64(std::string_view const input)
{
    return base64_codec::decode(input);
}

std::optional<std::string>
from_base64_safe(std::string_view const input)
{
    return base64_codec::decode_safe(input);
}

std::string
base64_codec::encode(std::string_view const input)
{
    if (input.empty()) {
        return {};
    }

    std::size_t const input_len = input.size();
    std::size_t const output_len = ((input_len + 2) / 3) * 4;

    std::string result;
    result.reserve(output_len);

    unsigned char const* data = reinterpret_cast<unsigned char const*>(input.data());
    std::size_t i = 0;

    // process 3-byte chunks
    for (; i + 2 < input_len; i += 3) {
        std::uint32_t const chunk = (static_cast<std::uint32_t>(data[i]) << 16)
                | (static_cast<std::uint32_t>(data[i + 1]) << 8)
                | static_cast<std::uint32_t>(data[i + 2]);

        result += encode_table[(chunk >> 18) & 0x3F];
        result += encode_table[(chunk >> 12) & 0x3F];
        result += encode_table[(chunk >> 6) & 0x3F];
        result += encode_table[chunk & 0x3F];
    }

    // handle remaining bytes
    if (i < input_len) {
        std::uint32_t chunk = static_cast<std::uint32_t>(data[i]) << 16;

        if (i + 1 < input_len) {
            chunk |= static_cast<std::uint32_t>(data[i + 1]) << 8;
            result += encode_table[(chunk >> 18) & 0x3F];
            result += encode_table[(chunk >> 12) & 0x3F];
            result += encode_table[(chunk >> 6) & 0x3F];
            result += '=';
        } else {
            result += encode_table[(chunk >> 18) & 0x3F];
            result += encode_table[(chunk >> 12) & 0x3F];
            result += "==";
        }
    }

    return result;
}

std::string
base64_codec::decode(std::string_view const input)
{
    if (input.empty()) {
        return {};
    }

    // remove padding for size calculation
    std::size_t input_len = input.size();
    std::size_t padding = 0;

    if (input_len >= 2) {
        if (input[input_len - 1] == '=') {
            padding++;
            if (input[input_len - 2] == '=') {
                padding++;
            }
        }
    }

    std::size_t const output_len = (input_len * 3) / 4 - padding;
    std::string result;
    result.reserve(output_len);

    std::uint32_t chunk = 0;
    int chunk_bits = 0;

    for (char const c : input) {
        if (c == '=')
            break; // stop at padding

        std::int8_t const value = decode_table[static_cast<unsigned char>(c)];
        if (value == -1) {
            // invalid character - skip it
            continue;
        }

        chunk = (chunk << 6) | value;
        chunk_bits += 6;

        if (chunk_bits >= 8) {
            chunk_bits -= 8;
            result += static_cast<char>((chunk >> chunk_bits) & 0xFF);
        }
    }

    return result;
}

std::optional<std::string>
base64_codec::decode_safe(std::string_view const input)
{
    if (input.empty()) {
        return std::string{};
    }

    // check if input length is valid (must be multiple of 4)
    if (input.size() % 4 != 0) {
        return std::nullopt;
    }

    // remove padding for size calculation
    std::size_t input_len = input.size();
    std::size_t padding = 0;

    if (input_len >= 2) {
        if (input[input_len - 1] == '=') {
            padding++;
            if (input[input_len - 2] == '=') {
                padding++;
            }
        }
    }

    std::size_t const output_len = (input_len * 3) / 4 - padding;
    std::string result;
    result.reserve(output_len);

    std::uint32_t chunk = 0;
    int chunk_bits = 0;

    for (char const c : input) {
        if (c == '=') {
            break; // stop at padding
        }

        std::int8_t const value = decode_table[static_cast<unsigned char>(c)];
        if (value == -1) {
            return std::nullopt; // invalid character
        }

        chunk = (chunk << 6) | value;
        chunk_bits += 6;

        if (chunk_bits >= 8) {
            chunk_bits -= 8;
            result += static_cast<char>((chunk >> chunk_bits) & 0xFF);
        }
    }

    return result;
}

} // namespace ws
