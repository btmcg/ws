#pragma once

#include <string>
#include <unordered_map>

namespace ws {

class tcp_echo_server
{
public:
    tcp_echo_server(int port);
    ~tcp_echo_server() noexcept;
    bool listen();

private:
    bool parse_request(std::string const&);
    bool validate_request_method_uri_and_version(std::string const&);
    bool validate_header_fields();

private:
    int sock_ = -1;
    int port_ = -1;
    std::unordered_map<std::string, std::string> header_fields_;
};

} // namespace ws
