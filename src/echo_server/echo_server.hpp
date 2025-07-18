#pragma once

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
    bool on_incoming_data(int fd) noexcept;

    /// Called on http request
    bool on_http_request(int fd, std::string const&) const noexcept;

    /// Called when a websocket upgrade request detected
    bool on_websocket_upgrade_request(int fd,
            std::unordered_map<std::string, std::string> const& header_fields) const noexcept;

private:
    bool validate_request_method_uri_and_version(std::string const&) const noexcept;
    bool validate_header_fields(
            std::unordered_map<std::string, std::string> const& header_fields) const noexcept;
    std::string generate_accept_key(std::string const&) const noexcept;
    bool send_websocket_accept(int fd, std::string const& sec_websocket_key) const noexcept;

private:
    static constexpr std::uint16_t ListenPort = 8000; ///< default listening port

private:
    int port_ = ListenPort;    ///< port to listen on
    int sockfd_ = -1;          ///< listening socket
    int epollfd_ = -1;         ///< epoll file descriptor
    std::vector<int> clients_; ///< list of client connected sockets
};

} // namespace ws
