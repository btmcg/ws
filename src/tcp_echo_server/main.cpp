#include "tcp_echo_server.hpp"
#include <print>

int
main()
{
    std::println("tcp echo server");
    tcp_echo_server tes;
    if (!tes.listen()) {
        std::println("exiting");
    }
    return 0;
}
