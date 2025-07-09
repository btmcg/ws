#include "tcp_echo_server.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <print>

tcp_echo_server::tcp_echo_server()
        : sock_(-1)
{
    std::println("tcp_echo_server instantiated");
    addrinfo hints;
    addrinfo* res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    ::getaddrinfo(NULL, "8080", &hints, &res);

    sock_ = ::socket(PF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        std::println("socket: {}: {}", std::strerror(errno), errno);
    }

    if (::bind(sock_, res->ai_addr, res->ai_addrlen) == -1) {
        std::println("bind: {}: {}", std::strerror(errno), errno);
    }
}

void
tcp_echo_server::listen()
{
    if (int rv = ::listen(sock_, 0); rv == -1) {
        std::println("listen: {}: {}", std::strerror(errno), errno);
    }

    std::println("tcp_echo_server listening...");

    sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    int new_fd = ::accept(sock_, reinterpret_cast<sockaddr*>(&their_addr), &addr_size);

    char buf[4096];
    ssize_t recvnbytes = ::recv(new_fd, buf, sizeof(buf), 0);
    while (recvnbytes != 0) {
        ::send(new_fd, buf, recvnbytes, 0);
        recvnbytes = ::recv(new_fd, buf, sizeof(buf), 0);
    }
}
