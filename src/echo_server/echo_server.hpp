#pragma once

#include "connection.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ws {

class echo_server
{
public:
    echo_server(int port);
    ~echo_server() noexcept;

    // No copies/moves
    echo_server(echo_server const&) = delete;
    echo_server(echo_server&&) = delete;
    echo_server& operator=(echo_server const&) = delete;
    echo_server&& operator=(echo_server&&) = delete;

    /// Start the server and begin listening on socket.
    /// \return \c false on error
    bool run();

private:
    /// Called on new connection
    /// \return \c false on error
    bool on_incoming_connection() noexcept;

    /// Called on incoming data
    /// \return \c false on error
    bool on_incoming_data(connection&) noexcept;

    /// Called on http request
    bool on_http_request(connection&) const noexcept;

    /// Called when a websocket upgrade request detected
    bool on_websocket_upgrade_request(
            connection&, std::unordered_map<std::string, std::string> const& header_fields) const;

    /// Called when receiving from a websocket that is already connected
    /// \return \c false on error
    bool on_websocket_frame(connection&);

    /// Called when a ping control frame received
    bool on_websocket_ping(connection&, std::span<std::uint8_t const> payload);

    /// Called when a pong control frame received
    bool on_websocket_pong(connection&, std::span<std::uint8_t const> payload);

    /// Called when a close frame received
    bool on_websocket_close(connection&);

    /// Called when a text frame received
    bool on_websocket_text_frame(connection&, std::string_view text_data);

private:
    bool validate_request_method_uri_and_version(std::string const&) const noexcept;
    bool validate_header_fields(
            std::unordered_map<std::string, std::string> const& header_fields) const noexcept;
    std::string generate_accept_key(std::string const&) const noexcept;
    bool send_websocket_accept(connection&, std::string const& sec_websocket_key) const noexcept;
    bool send_websocket_close(connection&);
    bool disconnect_and_cleanup_client(connection&);

private:
    static constexpr std::uint16_t ListenPort = 8000; ///< default listening port

private:
    int port_ = ListenPort;                       ///< port to listen on
    int sockfd_ = -1;                             ///< listening socket
    int epollfd_ = -1;                            ///< epoll file descriptor
    std::unordered_map<int, connection> clients_; ///< list of clients, keyed by socket fd
};

} // namespace ws
