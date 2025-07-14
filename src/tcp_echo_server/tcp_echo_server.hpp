#pragma once

#include <string>

class tcp_echo_server
{
public:
    tcp_echo_server(int port);
    ~tcp_echo_server() noexcept;
    bool listen();
    void parse_request(std::string const&);

private:
    int sock_ = -1;
};
