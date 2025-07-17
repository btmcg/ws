#pragma once

#include <string>

namespace ws {

class tcp_echo_server
{
public:
    tcp_echo_server(int port);
    ~tcp_echo_server() noexcept;
    bool listen();

private:
    void parse_request(std::string const&);
    bool parse_method(std::string const&);

private:
    int sock_ = -1;
    int port_ = -1;
};

} // namespace ws
