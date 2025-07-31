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

    ::freeaddrinfo(result);
    SPDLOG_INFO("successfully connected");

    while (true) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);
    }
    return EXIT_SUCCESS;
}

std::string
gen_sec_websocket_key()
{
}


bool
send_websocket_upgrade_request()
{
    std::string request
            = "GET / HTTP/1.1"
              "Host: localhost:8000"
              "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:141.0) Gecko/20100101 Firefox/141.0"
              "Accept: */*"
              "Accept-Language: en-US,en;q=0.5"
              "Accept-Encoding: gzip, deflate, br, zstd"
              "Sec-WebSocket-Version: 13"
              "Origin: null"
              "Sec-WebSocket-Extensions: permessage-deflate"
              "Sec-WebSocket-Key: wZsRzjw8tBLo+2Vruz472w=="
              "Sec-GPC: 1"
              "Connection: keep-alive, Upgrade"
              "Sec-Fetch-Dest: empty"
              "Sec-Fetch-Mode: websocket"
              "Sec-Fetch-Site: cross-site"
              "Pragma: no-cache"
              "Cache-Control: no-cache"
              "Upgrade: websocket";

    // expected response
    std::string response = "HTTP/1.1 101 Switching Protocols"
                           "Upgrade: websocket"
                           "Connection: Upgrade"
                           "Sec-WebSocket-Accept: cemlTOAnAhLvbZDQEKgJMBSF0D4=";
}

