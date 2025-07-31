#include <netdb.h> // ::getaddrinfo
#include <spdlog/spdlog.h>
#include <sys/socket.h> // ::connect, ::setsockopt, ::socket
#include <sys/types.h>
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <cstring> // std::memset, std::strerror
#include <thread>

int
main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    // [2025-07-17 11:10:13.674784] [info] [main.cpp:14] message
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%s:%#] %v");

    int port = 8000;
    if (argc == 2) {
        port = std::atoi(argv[1]);
    }

    addrinfo hints{};
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE;     // wildcard ip
    // hints.ai_protocol = 0;
    // hints.ai_canonname = nullptr;
    // hints.ai_addr = nullptr;
    // hints.ai_next = nullptr;

    // get local address
    addrinfo* result = nullptr;
    if (int rv = ::getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result); rv != 0) {
        SPDLOG_CRITICAL("getaddrinfo: {}", std::strerror(errno));
        return EXIT_FAILURE;
    }

    // get socket
    int sockfd = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd == -1) {
        SPDLOG_CRITICAL("socket: {}", std::strerror(errno));
    }

    // connect!
    if (int rv = ::connect(sockfd, result->ai_addr, result->ai_addrlen); rv == -1) {
        SPDLOG_CRITICAL("connect: {}", std::strerror(errno));
    }

    SPDLOG_INFO("successfully connected");


    while (true) {

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);
    }
    return EXIT_SUCCESS;
}
