#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ws {

// Convenience functions
std::string to_base64(std::string_view const input);
std::string from_base64(std::string_view const input);
std::optional<std::string> from_base64_safe(std::string_view const input);


/// \class  base64_codec
class base64_codec
{
private:
    static constexpr std::array<char, 64> encode_table
            = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
                    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
                    'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

    static constexpr std::array<std::int8_t, 256> decode_table = []() {
        std::array<std::int8_t, 256> table{};
        // Initialize all values to -1 (invalid)
        for (int i = 0; i < 256; ++i) {
            table[i] = -1;
        }
        // Set valid base64 characters
        for (int i = 0; i < 64; ++i) {
            table[static_cast<unsigned char>(encode_table[i])] = i;
        }
        return table;
    }();

public:
    /**
     * Encode a string to base64
     * @param input the input string to encode
     * @return base64 encoded string
     */
    static std::string encode(std::string_view const input);

    /**
     * Decode a base64 string
     * @param input the base64 string to decode
     * @return decoded string
     */
    static std::string decode(std::string_view const input);

    // TODO: is this needed?
    /**
     * Safely decode a base64 string with error handling
     * @param input the base64 string to decode
     * @return optional decoded string (nullopt if invalid input)
     */
    static std::optional<std::string> decode_safe(std::string_view const input);

    /**
     * Optimized encoding for known input sizes at compile time
     * @param input fixed-size array input
     * @return base64 encoded string
     */
    template <std::size_t N>
    static constexpr std::string encode_fixed(std::array<char, N> const& input);
};

// template implementation
template <std::size_t N>
constexpr std::string
base64_codec::encode_fixed(std::array<char, N> const& input)
{
    constexpr std::size_t output_len = ((N + 2) / 3) * 4;
    std::string result;
    result.reserve(output_len);

    for (std::size_t i = 0; i < N; i += 3) {
        std::uint32_t chunk = static_cast<std::uint32_t>(static_cast<unsigned char>(input[i]))
                << 16;

        if (i + 1 < N) {
            chunk |= static_cast<std::uint32_t>(static_cast<unsigned char>(input[i + 1])) << 8;
        }
        if (i + 2 < N) {
            chunk |= static_cast<std::uint32_t>(static_cast<unsigned char>(input[i + 2]));
        }

        result += encode_table[(chunk >> 18) & 0x3F];
        result += encode_table[(chunk >> 12) & 0x3F];

        if (i + 1 < N) {
            result += encode_table[(chunk >> 6) & 0x3F];
        } else {
            result += '=';
        }

        if (i + 2 < N) {
            result += encode_table[chunk & 0x3F];
        } else {
            result += '=';
        }
    }

    return result;
}

} // namespace ws
