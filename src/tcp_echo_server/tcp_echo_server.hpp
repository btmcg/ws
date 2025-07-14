#pragma once

class tcp_echo_server {
public:
    tcp_echo_server();
    ~tcp_echo_server() noexcept;
    bool listen();
private:
    int sock_ = -1;
};
