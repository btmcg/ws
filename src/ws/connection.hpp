#pragma once

#include "frame.hpp"
#include "util/byte_buffer.hpp"
#include <arpa/inet.h> // INET_ADDRSTRLEN
#include <cstdint>


namespace {
static constexpr std::size_t BufferSize = 1'048'576; ///< max size of connection incoming buffer
} // namespace

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
    byte_buffer<BufferSize> buf;
    char ip[INET_ADDRSTRLEN];
    std::uint16_t port = 0;

    ConnectionState conn_state = ConnectionState::Undefined;
    ParseState parse_state = ParseState::ReadingHeader;
    std::uint64_t bytes_needed = 2; // start with basic header
    std::uint64_t payload_bytes_read = 0;

    // fragmentation handling
    OpCode current_frame_type = OpCode::Continuation;
    bool is_fragmented_msg = false;
    std::uint64_t fragmented_payload_size = 0;
    std::vector<std::uint8_t> fragmented_payload;

    void
    reset_fragmentation()
    {
        current_frame_type = OpCode::Continuation;
        is_fragmented_msg = false;
        fragmented_payload_size = 0;
        fragmented_payload.clear();
    }
};

} // namespace ws
