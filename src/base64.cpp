#include "base64.hpp"

namespace ws {

std::string
to_base64(std::string_view input)
{
    return base64_encoder::encode(input);
}


std::string
base64_encoder::encode(std::string_view input)
{
    if (input.empty())
        return {};

    std::size_t const input_len = input.size();
    std::size_t const output_len = ((input_len + 2) / 3) * 4;

    std::string result;
    result.reserve(output_len);

    auto const* data = reinterpret_cast<unsigned char const*>(input.data());
    std::size_t i = 0;

    // Process 3-byte chunks
    for (; i + 2 < input_len; i += 3) {
        std::uint32_t const chunk = (static_cast<std::uint32_t>(data[i]) << 16)
                | (static_cast<std::uint32_t>(data[i + 1]) << 8)
                | static_cast<std::uint32_t>(data[i + 2]);

        result += table[(chunk >> 18) & 0x3F];
        result += table[(chunk >> 12) & 0x3F];
        result += table[(chunk >> 6) & 0x3F];
        result += table[chunk & 0x3F];
    }

    // Handle remaining bytes
    if (i < input_len) {
        std::uint32_t chunk = static_cast<std::uint32_t>(data[i]) << 16;

        if (i + 1 < input_len) {
            chunk |= static_cast<std::uint32_t>(data[i + 1]) << 8;
            result += table[(chunk >> 18) & 0x3F];
            result += table[(chunk >> 12) & 0x3F];
            result += table[(chunk >> 6) & 0x3F];
            result += '=';
        } else {
            result += table[(chunk >> 18) & 0x3F];
            result += table[(chunk >> 12) & 0x3F];
            result += "==";
        }
    }

    return result;
}

} // namespace ws
