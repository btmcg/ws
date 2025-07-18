#include "echo_server.hpp"
#include <spdlog/spdlog.h>
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS

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

    try {
        ws::echo_server server(port);
        if (!server.run()) {
            SPDLOG_CRITICAL("error: server shutdown with an error");
            return EXIT_FAILURE;
        }
    } catch (std::exception const& e) {
        SPDLOG_CRITICAL("error: exception: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        SPDLOG_CRITICAL("error: exception: ???");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
