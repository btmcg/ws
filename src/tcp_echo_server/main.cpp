#include "tcp_echo_server.hpp"
#include <spdlog/spdlog.h>
#include <print>

#undef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

int
main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::trace);

    // [2025-07-17 10:58:13.815773] [info] [tcp_echo_server.cpp:51] tcp_echo_server listening on port 8080...
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%s:%#] %v");
    SPDLOG_TRACE("tracetcp echo server");
    SPDLOG_DEBUG("debug tcp echo server");
    SPDLOG_INFO("info tcp echo server");

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
