#include "tcp_echo_server.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // ::close
#include <cstring>
#include <print>
#include <unordered_map>

tcp_echo_server::tcp_echo_server(int port)
        : sock_(-1)
{
    std::println("tcp_echo_server instantiated");
    sock_ = ::socket(PF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        std::println("socket: {}: {}", std::strerror(errno), errno);
    }

    sockaddr_in addr{.sin_family = AF_INET,
            .sin_port = std::byteswap(std::uint16_t(port)),
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

bool
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

        std::string str(buf.data(), static_cast<std::size_t>(bytes_read));
        std::println("{}", str);

        std::println("- - -");
        parse_request(str);
        std::println("- - -");
    }

    return true;
}

void
tcp_echo_server::parse_request(std::string const& req)
{
    std::unordered_map<std::string, std::string> header_map;
    std::string method;

    // extract command
    std::size_t pos = 0;
    std::size_t newline_pos = req.find("\r\n", pos);
    method = req.substr(pos, newline_pos - pos);
    std::println("method={}", method);
    pos = newline_pos + 2;

    while (pos < req.size()) {
        std::size_t colon_pos = req.find(':', pos);
        std::size_t newline_pos = req.find("\r\n", colon_pos);
        if (colon_pos == std::string::npos || newline_pos == std::string::npos) {
            break;
        }

        std::string header_key = req.substr(pos, colon_pos - pos);
        std::string header_val = req.substr(colon_pos + 2, newline_pos - colon_pos - 2);

        header_map.emplace(header_key, header_val);

        pos = newline_pos + 2;
    }

    for (auto& [key, val] : header_map) {
        std::println("key={}, val={}", key, val);
    }
}
