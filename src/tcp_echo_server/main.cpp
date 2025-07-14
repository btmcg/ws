#include "tcp_echo_server.hpp"
#include <print>

int
main(int argc, char* argv[])
{
    std::println("tcp echo server");


    int port = 8080;
    if (argc == 2) {
        port = std::atoi(argv[1]);
    }
    tcp_echo_server tes(port);
    if (!tes.listen()) {
        std::println("exiting");
    }
    return 0;
}
