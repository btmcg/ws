#include "tcp_echo_server.hpp"
#include <spdlog/spdlog.h>

int
main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    // [2025-07-17 11:10:13.674784] [info] [main.cpp:14] message
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%s:%#] %v");

    int port = 8080;
    if (argc == 2) {
        port = std::atoi(argv[1]);
    }

    ws::tcp_echo_server tes(port);
    if (!tes.listen()) {
        SPDLOG_ERROR("exiting");
    }
    return 0;
}
