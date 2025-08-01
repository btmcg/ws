#include <spdlog/spdlog.h>

int
main(int, char**)
{
    spdlog::set_level(spdlog::level::debug);

    // [2025-07-17 11:10:13.674784] [info] [main.cpp:14] message
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%s:%#] %v");

    SPDLOG_INFO("hello, world");
    return EXIT_SUCCESS;
}
