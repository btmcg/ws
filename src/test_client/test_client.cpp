#include "test_client.hpp"
#include "ws/frame_generator.hpp"
#include <netdb.h> // ::getaddrinfo
#include <spdlog/spdlog.h>
#include <sys/socket.h> // ::connect, ::send, ::setsockopt, ::socket
#include <sys/types.h>
#include <unistd.h> // ::gethostname
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

std::span<std::uint8_t>
test_client::recv()
{
    ::ssize_t nbytes = ::recv(sockfd_, buf_.write_ptr(), buf_.bytes_left(), /*flags=*/0);
    if (nbytes > 0) {
        return std::span(buf_.write_ptr(), buf_.bytes_unread());
    } else if (nbytes == 0) {
        return {};
    } else {
        SPDLOG_CRITICAL("recv: {}", std::strerror(errno));
        std::abort();
    }

    return {};
}

} // namespace ws
