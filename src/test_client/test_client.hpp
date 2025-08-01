#pragma once

#include "util/byte_buffer.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace ws {

class test_client
{
public:
    test_client(std::string const& ip, int port);
    ~test_client() noexcept;

    // no copies/moves
    test_client(test_client const&) = delete;
    test_client(test_client&&) = delete;
    test_client& operator=(test_client const&) = delete;
    test_client&& operator=(test_client&&) = delete;

    /// Start the client and connect to server.
    /// \return \c false on error
    bool connect();

    bool send_websocket_upgrade_request();

    bool send_simple_fragmented_message();
    bool send_large_fragmented_text_message();
    bool send_binary_fragmented_message();
    bool send_many_small_fragments();
    bool send_empty_fragments();
    bool send_single_byte_fragments();
    bool send_fragmented_message_with_interleaved_ping();

    // helper methods
    bool send_ping(std::string const& payload = "");
    bool expect_echo_response(std::string const& expected_text);
    bool expect_binary_echo_response(std::vector<std::uint8_t> const& expected_data);

    std::span<std::uint8_t const> recv();
    void mark_read(std::size_t);

private:
    std::string ip_;
    int port_ = 0;
    int sockfd_ = -1;
    byte_buffer<524'288> buf_;
};

} // namespace ws
