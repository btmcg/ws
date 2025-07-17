#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <string_view>


namespace ws {

std::string to_base64(std::string_view);

/*! \class  base64_encoder
 */
class base64_encoder
{
private:
    static constexpr std::array<char, 64> table
            = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
                    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
                    'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

public:
    static std::string encode(std::string_view);

    // Optimized version for known input sizes at compile time
    template <std::size_t N>
    static constexpr std::string
    encode_fixed(std::array<char, N> const& input)
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

            result += table[(chunk >> 18) & 0x3F];
            result += table[(chunk >> 12) & 0x3F];

            if (i + 1 < N) {
                result += table[(chunk >> 6) & 0x3F];
            } else {
                result += '=';
            }

            if (i + 2 < N) {
                result += table[chunk & 0x3F];
            } else {
                result += '=';
            }
        }

        return result;
    }
};

} // namespace ws
