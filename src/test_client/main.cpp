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

    SPDLOG_INFO("client connected");
    if (!client.send_websocket_upgrade_request()) {
        SPDLOG_CRITICAL("failed to send websocket upgrade request");
        return EXIT_FAILURE;
    }

    auto response = client.recv();
    SPDLOG_INFO("upgrade response: {} bytes", response.size());
    client.mark_read(response.size());

    client.send_simple_fragmented_message();

    auto echo = client.recv();
    SPDLOG_INFO("received echo: {} bytes", echo.size());

    if (!echo.empty()) {
        std::string echo_str(reinterpret_cast<char const*>(echo.data()), echo.size());
        SPDLOG_INFO("echo content: '{}'", echo_str);
    }

    client.mark_read(echo.size());

    return EXIT_SUCCESS;
}
