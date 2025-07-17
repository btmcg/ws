#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace ws {

class sha1
{
public:
    static constexpr std::size_t DIGEST_SIZE = 20;
    using digest_type = std::array<std::uint8_t, DIGEST_SIZE>;

    /**
     * Compute SHA-1 hash of input data
     * @param input The input data to hash
     * @return 20-byte SHA-1 digest
     */
    static digest_type hash(std::string_view input);

    /**
     * Compute SHA-1 hash and return as hexadecimal string
     * @param input The input data to hash
     * @return SHA-1 digest as lowercase hexadecimal string
     */
    static std::string hash_hex(std::string_view input);

    /**
     * Compute SHA-1 hash for binary data
     * @param data Pointer to binary data
     * @param length Length of data in bytes
     * @return 20-byte SHA-1 digest
     */
    static digest_type hash(const std::uint8_t* data, std::size_t length);

private:
    static constexpr std::uint32_t left_rotate(std::uint32_t value, std::size_t amount);
    static void process_chunk(std::array<std::uint32_t, 5>& hash, const std::uint8_t* chunk);
};

} // namespace ws
