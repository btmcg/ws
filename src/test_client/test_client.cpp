#include "test_client.hpp"
#include "ws/frame_generator.hpp"
#include <netdb.h> // ::getaddrinfo
#include <spdlog/spdlog.h>
#include <sys/socket.h> // ::connect, ::getsockname, ::send, ::setsockopt, ::socket
#include <sys/types.h>
#include <unistd.h> // ::gethostname
#include <bit>      // std::byteswap
#include <cstdlib>  // EXIT_FAILURE, EXIT_SUCCESS
#include <cstring>  // std::memset, std::strerror


namespace ws {

test_client::test_client(std::string const& ip, int port)
        : ip_(ip)
        , port_(port)
        , sockfd_(-1)
{
    /* empty */
}

test_client::~test_client() noexcept
{
    ::close(sockfd_);
}

bool
test_client::connect()
{
    addrinfo hints{};
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE;     // wildcard ip

    // get local address
    addrinfo* result = nullptr;
    if (int rv = ::getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &result); rv != 0) {
        SPDLOG_CRITICAL("getaddrinfo: {}", std::strerror(errno));
        return false;
    }

    sockfd_ = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd_ == -1) {
        SPDLOG_CRITICAL("socket: {}", std::strerror(errno));
        return false;
    }

    if (int rv = ::connect(sockfd_, result->ai_addr, result->ai_addrlen); rv == -1) {
        SPDLOG_CRITICAL("connect: {}", std::strerror(errno));
        return false;
    }

    ::freeaddrinfo(result);
    return true;
}

std::span<std::uint8_t const>
test_client::recv()
{
    ::ssize_t nbytes = ::recv(sockfd_, buf_.write_ptr(), buf_.bytes_left(), /*flags=*/0);
    if (nbytes > 0) {
        buf_.bytes_written(nbytes);
        return std::span(buf_.read_ptr(), buf_.bytes_unread());
    } else if (nbytes == 0) {
        return {}; // Connection closed
    } else {
        SPDLOG_CRITICAL("recv: {}", std::strerror(errno));
        std::abort();
    }

    return {};
}

void
test_client::mark_read(std::size_t nbytes)
{
    buf_.bytes_read(nbytes);
}

bool
test_client::send_websocket_upgrade_request()
{
    SPDLOG_DEBUG("sending websocket upgrade request");

    char hostname[HOST_NAME_MAX]; // Buffer to store the hostname
    if (int rv = ::gethostname(hostname, HOST_NAME_MAX); rv != 0) {
        SPDLOG_CRITICAL("gethostname: {}", std::strerror(errno));
        return false;
    }

    sockaddr_in sa;
    socklen_t socklen = static_cast<socklen_t>(sizeof(sa));
    if (int rv = ::getsockname(sockfd_, reinterpret_cast<sockaddr*>(&sa), &socklen); rv == -1) {
        SPDLOG_CRITICAL("recv: {}", std::strerror(errno));
        return false;
    }

    int const local_port = std::byteswap(sa.sin_port);

    ws::frame_generator frame_gen;

    std::string const websocket_key = frame_gen.generate_websocket_key();
    std::string const request = std::format("GET / HTTP/1.1\r\n"
                                            "Host: {}:{}\r\n"
                                            "Upgrade: websocket\r\n"
                                            "Connection: Upgrade\r\n"
                                            "Sec-WebSocket-Key: {}\r\n"
                                            "Sec-WebSocket-Version: 13\r\n"
                                            "\r\n",
            hostname, local_port, websocket_key);

    if (ssize_t nbytes = ::send(sockfd_, request.data(), request.size(), /*flags=*/0);
            nbytes != static_cast<ssize_t>(request.size())) {
        SPDLOG_CRITICAL("send: {}", std::strerror(errno));
        return false;
    }

    return true;
}

bool
test_client::send_simple_fragmented_message()
{
    ssize_t nbytes = -1;

    std::string part1 = "hello";
    std::string part2 = " world!";

    // first fragment (text frame, FIN=false)
    auto frame1 = ws::frame_generator{}.text(part1, /*fin=*/false, /*mask=*/true);
    if (nbytes = ::send(sockfd_, frame1.data().data(), frame1.size(), /*flags=*/0);
            nbytes != static_cast<ssize_t>(frame1.size())) {
        SPDLOG_CRITICAL("send: {}", std::strerror(errno));
        return false;
    }
    SPDLOG_DEBUG("send_simple_fragmented_message: sent fragment 1: {} bytes", nbytes);

    // second fragment (continuation frame, FIN=true)
    auto frame2 = ws::frame_generator{}.continuation(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(part2.data()), part2.size()),
            /*fin=*/true, /*mask=*/true);
    if (nbytes = ::send(sockfd_, frame2.data().data(), frame2.size(), /*flags=*/0);
            nbytes != static_cast<ssize_t>(frame2.size())) {
        SPDLOG_CRITICAL("send: {}", std::strerror(errno));
        return false;
    }

    SPDLOG_DEBUG("send_simple_fragmented_message: sent fragment 2: {} bytes", nbytes);
    return true;
}

} // namespace ws
