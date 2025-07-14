#include "tcp_echo_server.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // ::close
#include <cstring>
#include <print>
#include <string_view>

tcp_echo_server::tcp_echo_server()
        : sock_(-1)
{
    std::println("tcp_echo_server instantiated");
    sock_ = ::socket(PF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        std::println("socket: {}: {}", std::strerror(errno), errno);
    }

    sockaddr_in addr{.sin_family = AF_INET,
            .sin_port = std::byteswap(std::uint16_t(8080)),
            .sin_addr = {INADDR_ANY},
            .sin_zero = 0};

    if (::bind(sock_, reinterpret_cast<sockaddr const*>(&addr), sizeof(addr)) == -1) {
        std::println("bind: {}: {}", std::strerror(errno), errno);
    }
}

tcp_echo_server::~tcp_echo_server()
{
    ::close(sock_);
}

void
tcp_echo_server::listen()
{
    if (int rv = ::listen(sock_, 5); rv == -1) {
        std::println("listen: {}: {}", std::strerror(errno), errno);
    }

    std::println("tcp_echo_server listening...");


    while (true) {
        sockaddr_storage their_addr;
        socklen_t addr_size = sizeof(their_addr);
        int new_fd = ::accept(sock_, reinterpret_cast<sockaddr*>(&their_addr), &addr_size);

        std::array<char, 4096> buf{};
        ssize_t const bytes_read = ::recv(new_fd, buf.data(), buf.size(), 0);
        if (bytes_read == 0) {
            ::close(sock_);
            continue;
        }

        std::string_view sv(buf.data(), static_cast<std::size_t>(bytes_read));
        std::println("{}", sv);
    }

    return true;
}
