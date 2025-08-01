#pragma once

#include "util/byte_buffer.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>

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

    std::span<std::uint8_t const> recv();
    void mark_read(std::size_t);

private:
    std::string ip_;
    int port_ = 0;
    int sockfd_ = -1; ///< listening socket
    byte_buffer<524'288> buf_;
};

} // namespace ws
