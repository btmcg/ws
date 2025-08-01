#pragma once

#include "frame.hpp"
#include "util/byte_buffer.hpp"
#include <arpa/inet.h> // INET_ADDRSTRLEN
#include <cstdint>
#include <format>


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

struct connection
{
    int sockfd;
    byte_buffer<BufferSize> buf;
    char ip[INET_ADDRSTRLEN];
    std::uint16_t port = 0;

    ConnectionState conn_state = ConnectionState::Undefined;

    // fragmentation handling
    OpCode current_frame_type = OpCode::Continuation;
    bool is_fragmented_msg = false;
    std::uint64_t fragmented_payload_size = 0;
    std::vector<std::uint8_t> fragmented_payload;
    std::size_t fragments_received = 0;

    void
    reset_fragmentation()
    {
        current_frame_type = OpCode::Continuation;
        is_fragmented_msg = false;
        fragmented_payload_size = 0;
        fragmented_payload.clear();
        fragments_received = 0;
    }

    std::string
    fragmentation_status() const
    {
        if (!is_fragmented_msg) {
            return "not_fragmented";
        }
        return std::format("fragmented(type={}, fragments={}, size={})",
                static_cast<int>(current_frame_type), fragments_received, fragmented_payload_size);
    }
};

} // namespace ws
