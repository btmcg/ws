#pragma once

#include "websocket_frame.hpp"
#include <cstdint>
#include <vector>

namespace ws {

enum class ConnectionState : std::uint8_t
{
    Http,
    Websocket,
    WebsocketClosing
};

enum class ParseState : std::uint8_t
{
    ReadingHeader,
    ReadingExtendedLen16,
    ReadingExtendedLen64,
    ReadingMask,
    ReadingPayload
};


struct connection
{
    std::vector<std::uint8_t> buf;
    ConnectionState conn_state = ConnectionState::Http;
    ParseState parse_state = ParseState::ReadingHeader;
    websocket_frame current_frame;
    std::uint64_t bytes_needed = 2; // start with basic header
    std::uint64_t payload_bytes_read = 0;
};

} // namespace ws
