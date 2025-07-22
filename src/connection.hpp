#pragma once

#include "websocket_frame.hpp"
#include <arpa/inet.h> // INET_ADDRSTRLEN
#include <cstdint>
#include <vector>

namespace ws {

enum class ConnectionState : std::uint8_t
{
    TcpConnected,
    Http,
    WebSocket,
    WebSocketClosing,
    Undefined
};

enum class ParseState : std::uint8_t
{
    ReadingHeader,
    ReadingExtendedLen16,
    ReadingExtendedLen64,
    ReadingMask,
    ReadingPayload,
    Undefined
};

struct connection
{
    int sockfd;
    std::vector<std::uint8_t> buf;
    websocket_frame current_frame;
    char ip[INET_ADDRSTRLEN];
    std::uint16_t port = 0;

    ConnectionState conn_state = ConnectionState::Undefined;
    ParseState parse_state = ParseState::ReadingHeader;
    std::uint64_t bytes_needed = 2; // start with basic header
    std::uint64_t payload_bytes_read = 0;
};

} // namespace ws
