#pragma once

class tcp_echo_server {
public:
    tcp_echo_server();
    void listen();
private:
    int sock_ = -1;
};
