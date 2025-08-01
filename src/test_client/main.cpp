#include "test_client/test_client.hpp"
#include <spdlog/spdlog.h>
#include <cstdint>
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <cstring> // std::memset, std::strerror
#include <span>

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

    ws::test_client client("127.0.0.1", port);

    if (!client.connect()) {
        SPDLOG_CRITICAL("client failed to connect");
        return EXIT_FAILURE;
    }

    SPDLOG_INFO("client connected");

    // ws::frame_generator f_gen;

    // std::string const websocket_key = f_gen::generate_websocket_key();
    // std::string const request = std::format("GET / HTTP/1.1\r\n"
    //                                         "Host: {}:{}\r\n"
    //                                         "Upgrade: websocket\r\n"
    //                                         "Connection: Upgrade\r\n"
    //                                         "Sec-WebSocket-Key: {}\r\n"
    //                                         "Sec-WebSocket-Version: 13\r\n"
    //                                         "\r\n",
    //         hostname, port, websocket_key);


    // if (ssize_t nbytes = ::send(sockfd, request.data(), request.size(), /*flags=*/0);
    //         nbytes != request.size()) {
    //     SPDLOG_CRITICAL("send: {}", std::strerror(errno));
    //     return EXIT_FAILURE;
    // }

    std::span<std::uint8_t> buf = client.recv();
    SPDLOG_INFO("received {} bytes", buf.size());

    return EXIT_SUCCESS;
}
