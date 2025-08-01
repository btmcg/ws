#include "echo_server.hpp"
#include "util/base64_codec.hpp"
#include "util/sha1.hpp"
#include "util/str_utils.hpp"
#include "ws/connection.hpp"
#include "ws/connection_fmt.hpp"
#include "ws/frame.hpp"
#include "ws/frame_fmt.hpp"
#include "ws/frame_generator.hpp"
#include <arpa/inet.h> // ::inet_ntop
#include <fcntl.h>     // ::fcntl
#include <netdb.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/socket.h> // ::setsockopt
#include <sys/types.h>
#include <unistd.h>  // ::close
#include <algorithm> // std::find_if
#include <cassert>
#include <cstdlib> // std::abort
#include <cstring> // std::memset, std::strerror
#include <print>
#include <span>
#include <unordered_map>
#include <vector>


namespace ws {
namespace {
    static constexpr int ListenBacklog = 10;     ///< max num of pending connections
    static constexpr int EpollMaxEvents = 20;    ///< max num of pending epoll events
    static constexpr int EpollTimeoutMsecs = 10; ///< num of milliseconds to block on epoll_wait
    static constexpr std::string_view MagicGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
} // namespace


void
print_hexdump(void const* data, std::size_t size)
{
    auto const* bytes = static_cast<std::uint8_t const*>(data);

    for (std::size_t i = 0; i < size; i += 16) {
        std::print("{:08X}  ", static_cast<unsigned>(i)); // Offset

        // Hex bytes
        for (std::size_t j = 0; j < 16; ++j) {
            if (i + j < size) {
                std::print("{:02X} ", bytes[i + j]);
            } else {
                std::print("   "); // Padding
            }
            if (j == 7)
                std::print(" "); // Extra space in middle
        }

        std::print(" |");

        // ASCII view
        for (std::size_t j = 0; j < 16 && i + j < size; ++j) {
            unsigned char c = bytes[i + j];
            std::print("{}", std::isprint(c) ? static_cast<char>(c) : '.');
        }

        std::print("|\n");
    }
}

echo_server::echo_server(int port)
        : port_(port)
        , sockfd_(-1)
        , epollfd_(-1)
        , clients_()
{
    addrinfo hints{};

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE;     // wildcard ip

    // get local address
    addrinfo* result = nullptr;
    if (int rv = ::getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &result); rv != 0) {
        throw std::runtime_error(std::string("getaddrinfo: ") + std::strerror(errno));
    }

    // get socket
    sockfd_ = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd_ == -1) {
        throw std::runtime_error(std::string("socket: ") + std::strerror(errno));
    }

    // allow for socket reuse
    int const yes = 1;
    if (int rv = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); rv == -1) {
        throw std::runtime_error(std::string("setsockopt (SO_REUSEADDR): ") + std::strerror(errno));
    }
    if (int rv = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)); rv == -1) {
        throw std::runtime_error(std::string("setsockopt (SO_REUSEPORT): ") + std::strerror(errno));
    }

    // bind
    if (int rv = ::bind(sockfd_, result->ai_addr, result->ai_addrlen); rv == -1) {
        throw std::runtime_error(std::string("bind: ") + std::strerror(errno));
    }

    ::freeaddrinfo(result);

    // set socket as non-blocking
    if (int rv = ::fcntl(sockfd_, F_SETFL, O_NONBLOCK); rv == -1) {
        throw std::runtime_error(std::string("fcntl (O_NONBLOCK): ") + std::strerror(errno));
    }

    // get epoll fd
    epollfd_ = ::epoll_create1(0);
    if (epollfd_ == -1) {
        throw std::runtime_error(std::string("epoll_create1: ") + std::strerror(errno));
    }
}

echo_server::~echo_server() noexcept
{
    ::close(sockfd_);
    ::close(epollfd_);

    for (auto& [sock, conn] : clients_) {
        ::close(sock);
    }
}

bool
echo_server::run()
{
    // start listening
    if (int rv = ::listen(sockfd_, ListenBacklog); rv == -1) {
        SPDLOG_CRITICAL("error: listen {}: {}", std::strerror(errno), errno);
        return false;
    }
    SPDLOG_INFO("listening on port {}", port_);

    // Add our listening socket to epoll.
    epoll_event event{};
    event.data.fd = sockfd_;
    event.events = (EPOLLIN | EPOLLET);
    if (int rv = ::epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd_, &event); rv == -1) {
        SPDLOG_CRITICAL("error: epoll_ctl: {} {}", std::strerror(errno), errno);
        return false;
    }

    epoll_event events[EpollMaxEvents];
    for (;;) {
        int const num_events = ::epoll_wait(
                epollfd_, static_cast<epoll_event*>(events), EpollMaxEvents, EpollTimeoutMsecs);
        if (num_events == -1) {
            SPDLOG_CRITICAL("error: epoll_wait: {} {}", std::strerror(errno), errno);
            return false;
        }

        for (int i = 0; i < num_events; ++i) {
            // Check for flag that we aren't listening for. Not sure if this is necessary.
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
                SPDLOG_ERROR("error: unexpected event on fd {}\n", i);

                auto itr = clients_.find(events[i].data.fd);
                if (itr == clients_.end()) {
                    int fd = events[i].data.fd;
                    SPDLOG_CRITICAL("failed to find client entry for fd={}", fd);
                    std::abort();
                }

                clients_.erase(itr);
                ::close(events[i].data.fd);
                continue;
            }

            if (events[i].data.fd == sockfd_) {
                bool const status = on_incoming_connection();
                if (!status) {
                    return false;
                }
            } else {
                int fd = events[i].data.fd;
                auto itr = clients_.find(fd);
                if (itr == clients_.end()) {
                    SPDLOG_CRITICAL("failed to find client entry for fd={}", fd);
                    std::abort();
                }

                bool const status = on_incoming_data(itr->second);
                if (!status) {
                    return false;
                }
            }
        } // for each event
    } // main event loop

    return true;
}

bool
echo_server::on_incoming_connection() noexcept
{
    // accept the connection
    sockaddr_storage their_addr{};
    socklen_t addr_size = sizeof(their_addr);
    int const accepted_sock
            = ::accept(sockfd_, reinterpret_cast<sockaddr*>(&their_addr), &addr_size);
    if (accepted_sock == -1) {
        if (errno == EAGAIN) {
            return true; // not an error
        }
        SPDLOG_CRITICAL("error: accept: {} {}", std::strerror(errno), errno);
        return false;
    }

    connection conn{};
    conn.conn_state = ConnectionState::TcpConnected;
    conn.sockfd = accepted_sock;

    auto const* sin = reinterpret_cast<sockaddr_in const*>(&their_addr);
    char const* ip = nullptr;
    if (ip = ::inet_ntop(AF_INET, &(sin->sin_addr), conn.ip, sizeof(conn.ip)); ip == nullptr) {
        SPDLOG_CRITICAL("inet_ntop: {}: {}", std::strerror(errno), errno);
        std::abort();
    }
    conn.port = sin->sin_port;

    // add the new fd to epoll
    epoll_event event{};
    event.events = (EPOLLIN | EPOLLET);
    event.data.fd = accepted_sock;
    if (int rv = ::epoll_ctl(epollfd_, EPOLL_CTL_ADD, accepted_sock, &event); rv == -1) {
        SPDLOG_CRITICAL("error: epoll_ctl (EPOLL_CTL_ADD): {} {}", std::strerror(errno), errno);
        return false;
    }

    // successfully connected. add client entry
    clients_.emplace(accepted_sock, std::move(conn));
    SPDLOG_INFO("client connected: {}", conn);

    return true;
}

bool
echo_server::on_incoming_data(connection& conn) noexcept
{
    // shift once we're past the 75% mark of capacity
    if (conn.buf.bytes_left() / conn.buf.capacity() < 0.25) {
        conn.buf.shift();
    }

    ssize_t const nbytes
            = ::recv(conn.sockfd, conn.buf.write_ptr(), conn.buf.bytes_left(), /*flags=*/0);
    if (nbytes == -1) {
        SPDLOG_CRITICAL("error: recv: {} {}", std::strerror(errno), errno);
        return false;
    }
    conn.buf.bytes_written(nbytes);

    // client disconnected
    if (nbytes == 0) {
        SPDLOG_INFO("client on fd {} disconnected", conn.sockfd);
        disconnect_and_cleanup_client(conn);
        return true;
    }

    if (conn.conn_state == ConnectionState::WebSocket) {
        if (!on_websocket_frame(conn)) {
            SPDLOG_ERROR("on_websocket_frame returned false");
        }
    } else {
        if (!on_http_request(conn)) {
            SPDLOG_ERROR("on_http_request returned false");
        }
    }
    return true;
}

bool
echo_server::on_http_request(connection& conn) const noexcept
{
    conn.conn_state = ConnectionState::Http;
    std::string_view req(
            reinterpret_cast<char const*>(conn.buf.read_ptr()), conn.buf.bytes_unread());
    SPDLOG_DEBUG("received http request:\n{}", req);
    conn.buf.bytes_read(conn.buf.bytes_unread());

    if (req.starts_with("GET")) {
        SPDLOG_DEBUG("on_http_request: GET");
    } else {
        // TODO
        SPDLOG_CRITICAL("did not receive GET request");
        std::abort();
    }

    std::string method;
    std::unordered_map<std::string, std::string> header_fields;

    // extract command
    std::size_t pos = 0;
    std::size_t newline_pos = req.find("\r\n", pos);
    method = req.substr(pos, newline_pos - pos);
    SPDLOG_DEBUG("method={}", method);
    pos = newline_pos + 2;

    if (!validate_request_method_uri_and_version(method)) {
        SPDLOG_ERROR("request method, uri, and version validation failed");
        return false;
    }

    while (pos < req.size()) {
        std::size_t colon_pos = req.find(':', pos);
        std::size_t newline_pos = req.find("\r\n", colon_pos);
        if (colon_pos == std::string::npos || newline_pos == std::string::npos) {
            break;
        }

        std::string header_key = std::string(req.substr(pos, colon_pos - pos));
        std::string header_val
                = std::string(req.substr(colon_pos + 1, newline_pos - colon_pos - 1));

        trim(header_key);
        trim(header_val);
        header_fields.emplace(to_lower(header_key), header_val);

        pos = newline_pos + 2;
    }

    if (header_fields.contains("upgrade")) {
        if (!on_websocket_upgrade_request(conn, header_fields)) {
            SPDLOG_ERROR("invalid websocket upgrade request");
            return false;
        }
    } else {
        // TODO
    }

    // echo
    // ::ssize_t const bytes_sent = ::send(fd, &buf, static_cast<std::size_t>(bytes_recvd), 0);
    // if (bytes_sent == -1) {
    //     SPDLOG_CRITICAL("error: send: {} {}", std::strerror(errno), errno);
    //     return false;
    // }

    // buf[bytes_recvd - 1] = '\0'; // NOLINT
    // SPDLOG_INFO("[on_incoming_data] fd={}, buf={}\n", fd, buf);

    return true;
}

bool
echo_server::on_websocket_upgrade_request(
        connection& conn, std::unordered_map<std::string, std::string> const& header_fields) const
{
    if (!validate_header_fields(header_fields)) {
        SPDLOG_ERROR("header fields validation failed");
        return false;
    }

    auto itr = header_fields.find("sec-websocket-key");
    assert(itr != header_fields.end());

    if (!send_websocket_accept(conn, itr->second)) {
        SPDLOG_ERROR("failed to send websocket accept");
        return false;
    } else {
        SPDLOG_INFO("response sent");
    }


    return true;
}

bool
echo_server::validate_request_method_uri_and_version(
        std::string const& method_and_ver) const noexcept
{
    // According to RFC 2616, section 5.1.1
    // The Method token indicates the method to be performed on the resource identified by
    // the Request-URI. The method is case-sensitive.
    std::string str = to_upper(method_and_ver);

    std::vector<std::string> tokens = tokenize(str);
    if (tokens.size() != 3) {
        SPDLOG_CRITICAL("Invalid number of tokens: [{}]", str);
        return false;
    }

    std::string method = tokens[0];
    std::string uri = tokens[1];
    std::string version = tokens[2];

    if (method != "GET") {
        SPDLOG_CRITICAL("unsupported method: {}", tokens[0]);
        return false;
    }

    if (version != "HTTP/1.1") {
        SPDLOG_CRITICAL("unsupported version: {}", tokens[2]);
        return false;
    }

    return true;
}

bool
echo_server::validate_header_fields(
        std::unordered_map<std::string, std::string> const& header_fields) const noexcept
{
    // According to RFC 7230, section 3.2:
    // Each header field consists of a case-insensitive field name
    // followed by a colon (":"), optional leading whitespace, the field
    // value, and optional trailing whitespace.
    for (auto& [key, val] : header_fields) {
        SPDLOG_DEBUG("header_fields key={}, val={}", key, val);
    }

    // Upgrade
    {
        static constexpr char key[] = "upgrade";
        auto itr = header_fields.find(key);
        if (itr == header_fields.end()) {
            SPDLOG_CRITICAL("missing '{}' field", key);
            return false;
        }
        auto const& val = itr->second;
        if (val != "websocket") {
            SPDLOG_CRITICAL("invalid '{}' value: [{}], expected 'websocket'", key, val);
            return false;
        }
    }

    // Connection
    {
        static constexpr char key[] = "connection";
        auto itr = header_fields.find(key);
        if (itr == header_fields.end()) {
            SPDLOG_CRITICAL("missing '{}' field", key);
            return false;
        }
        auto const& val = itr->second;
        if (!val.contains("Upgrade")) {
            SPDLOG_CRITICAL("invalid '{}' value: [{}], expected 'Upgrade'", key, val);
            return false;
        }
    }

    // Sec-Websocket-Version
    {
        static constexpr char key[] = "sec-websocket-version";
        auto itr = header_fields.find(key);
        if (itr == header_fields.end()) {
            SPDLOG_CRITICAL("missing '{}' field", key);
            return false;
        }
        auto const& val = itr->second;
        if (val.empty()) {
            SPDLOG_CRITICAL("invalid '{}' value: [{}]", key, val);
            return false;
        }
    }

    // Sec-Websocket-Key
    {
        static constexpr char key[] = "sec-websocket-key";
        auto itr = header_fields.find(key);
        if (itr == header_fields.end()) {
            SPDLOG_CRITICAL("missing '{}' field", key);
            return false;
        }
        auto const& val = itr->second;
        if (val.empty()) {
            SPDLOG_CRITICAL("invalid '{}' value: [{}]", key, val);
            return false;
        }
    }

    return true;
}

std::string
echo_server::generate_accept_key(std::string const& key) const noexcept
{
    SPDLOG_DEBUG("key={}", key);
    std::string const concat = key + std::string(MagicGuid);
    SPDLOG_DEBUG("concat={}", concat);

    // Get the raw SHA-1 hash bytes (not hex string!)
    auto const sha1_digest = sha1::hash(concat);
    SPDLOG_DEBUG("sha1_digest raw bytes computed");

    // base64-encode the raw bytes directly
    std::string_view raw_bytes(
            reinterpret_cast<const char*>(sha1_digest.data()), sha1_digest.size());
    std::string b64_hash = to_base64(raw_bytes);
    SPDLOG_DEBUG("b64_hash={}", b64_hash);
    return b64_hash;
}

bool
echo_server::send_websocket_accept(
        connection& conn, std::string const& sec_websocket_key) const noexcept
{
    conn.conn_state = ConnectionState::WebSocket;
    std::string accept_key = generate_accept_key(sec_websocket_key);
    std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Accept: "
            + accept_key + "\r\n\r\n";
    SPDLOG_DEBUG("response=\n{}", response);

    SPDLOG_DEBUG("sending {} bytes", response.size());
    ssize_t nbytes
            = ::send(conn.sockfd, response.c_str(), static_cast<ssize_t>(response.size()), 0);
    if (nbytes == -1) {
        SPDLOG_CRITICAL("send: {}: {}", std::strerror(errno), errno);
        return false;
    }

    return true;
}


bool
echo_server::on_websocket_frame(connection& conn)
{
    SPDLOG_DEBUG("on_websocket_frame: bytes_unread={}", conn.buf.bytes_unread());

    ws::frame frame;
    ParseResult result = frame.parse_from_buffer(conn.buf.read_ptr(), conn.buf.bytes_unread());

    switch (result) {
        case ParseResult::NeedMoreData:
            SPDLOG_DEBUG("need more data for complete frame");
            return true; // wait for more data

        case ParseResult::InvalidFrame:
            SPDLOG_ERROR("invalid WebSocket frame received");
            disconnect_and_cleanup_client(conn);
            return false; // invalid frame - close connection

        case ParseResult::Success:
        default:
            break;
    }

    SPDLOG_DEBUG("parsed frame: fin={}, op_code={}, masked={}, payload_len={}, header_size={}",
            frame.fin(), frame.op_code(), frame.masked(), frame.payload_len(), frame.header_size());

    switch (frame.op_code()) {
        case OpCode::Close: {
            on_websocket_close(conn);
            conn.buf.bytes_read(frame.total_size());
        } break;

        case OpCode::Ping: {
            on_websocket_ping(conn, frame.get_payload_data());
            conn.buf.bytes_read(frame.total_size());
        } break;

        case OpCode::Pong: {
            on_websocket_pong(conn, frame.get_payload_data());
            conn.buf.bytes_read(frame.total_size());
        } break;

        case OpCode::Text:
        case OpCode::Binary:
        case OpCode::Continuation:
            return on_websocket_data_frame(conn, frame);

        default:
            SPDLOG_WARN("received frame with unsupported opcode: {}",
                    static_cast<int>(frame.op_code()));
            conn.buf.bytes_read(frame.total_size());
            break;
    }
    return true;
}

bool
echo_server::on_websocket_ping(connection& conn, std::span<std::uint8_t const> payload)
{
    SPDLOG_INFO("received ping frame");

    auto frame = frame_generator{}.pong(payload);

    SPDLOG_DEBUG("sending {} bytes", frame.size());
    ssize_t nbytes = ::send(
            conn.sockfd, frame.data().data(), static_cast<ssize_t>(frame.size()), /*flags=*/0);
    if (nbytes == -1) {
        SPDLOG_CRITICAL("send: {}: {}", std::strerror(errno), errno);
        return false;
    }

    return true;
}

bool
echo_server::on_websocket_pong(connection&, std::span<std::uint8_t const> payload)
{
    if (payload.empty()) {
        SPDLOG_INFO("received pong frame");
    } else {
        SPDLOG_INFO("received pong frame with payload of {} bytes", payload.size());
    }
    return true;
}

bool
echo_server::on_websocket_close(connection& conn)
{
    SPDLOG_INFO("received close frame");
    conn.conn_state = ConnectionState::WebSocketClosing;

    auto frame = frame_generator{}.close();

    SPDLOG_DEBUG("sending {} bytes", frame.size());
    ssize_t nbytes = ::send(
            conn.sockfd, frame.data().data(), static_cast<ssize_t>(frame.size()), /*flags=*/0);
    if (nbytes == -1) {
        SPDLOG_CRITICAL("send: {}: {}", std::strerror(errno), errno);
        return false;
    }

    disconnect_and_cleanup_client(conn);
    return true;
}

bool
echo_server::on_websocket_data_frame(connection& conn, frame const& frame)
{
    OpCode opcode = frame.op_code();

    if (opcode == OpCode::Continuation) {
        if (!conn.is_fragmented_msg) {
            SPDLOG_ERROR("received continuation frame without prior fragmented message");
            disconnect_and_cleanup_client(conn);
            return false;
        }
    } else { // text or binary
        if (conn.is_fragmented_msg) {
            SPDLOG_ERROR("received {} frame while processing fragmented message", opcode);
            disconnect_and_cleanup_client(conn);
            return false;
        }
    }

    if (frame.fin()) {
        // this is either a complete message or the final fragment
        if (conn.is_fragmented_msg) {
            // final fragment - process the complete accumulated message
            return process_complete_fragmented_message(conn, frame);
        } else {
            // complete single-frame message
            return process_single_frame_message(conn, frame);
        }
    } else {
        // this is the start of a fragmented message or a continuation fragment
        if (!conn.is_fragmented_msg) {
            conn.current_frame_type = opcode;
            conn.is_fragmented_msg = true;
            conn.fragmented_payload_size = 0;
            conn.fragmented_payload.clear();
            SPDLOG_DEBUG("starting fragmented {} message", static_cast<int>(opcode));
        }

        // store the payload data and consume the entire frame
        auto payload = frame.get_payload_data();
        conn.fragmented_payload.insert(
                conn.fragmented_payload.end(), payload.begin(), payload.end());
        conn.fragmented_payload_size += frame.payload_len();

        conn.buf.bytes_read(frame.total_size());

        SPDLOG_DEBUG("accumulated fragment: {} bytes this frame, {} bytes total",
                frame.payload_len(), conn.fragmented_payload_size);

        return true;
    }
}

bool
echo_server::process_single_frame_message(connection& conn, frame const& frame)
{
    // process a complete single-frame message
    if (frame.op_code() == OpCode::Text) {
        auto payload = frame.get_text_payload();
        if (!payload) {
            SPDLOG_ERROR("received text frame with bad payload");
            return false;
        }
        on_websocket_text_frame(conn, payload.value());
    } else if (frame.op_code() == OpCode::Binary) {
        on_websocket_binary_frame(conn, frame.get_payload_data());
    }

    send_echo(conn, frame.get_payload_data(), frame.op_code());
    conn.buf.bytes_read(frame.total_size());

    return true;
}

bool
echo_server::process_complete_fragmented_message(connection& conn, frame const& final_frame)
{
    // add the final frame's payload to our accumulated data
    auto final_payload = final_frame.get_payload_data();
    conn.fragmented_payload.insert(
            conn.fragmented_payload.end(), final_payload.begin(), final_payload.end());

    SPDLOG_DEBUG("completed fragmented message: {} total bytes", conn.fragmented_payload.size());

    // process the complete message
    if (conn.current_frame_type == OpCode::Text) {
        std::string complete_text(reinterpret_cast<char const*>(conn.fragmented_payload.data()),
                conn.fragmented_payload.size());
        on_websocket_text_frame(conn, complete_text);
    } else if (conn.current_frame_type == OpCode::Binary) {
        std::span<const std::uint8_t> complete_data(conn.fragmented_payload);
        on_websocket_binary_frame(conn, complete_data);
    }

    // send echo response
    std::span<const std::uint8_t> echo_data(conn.fragmented_payload);
    send_echo(conn, echo_data, conn.current_frame_type);

    // consume the final frame
    conn.buf.bytes_read(final_frame.total_size());

    conn.reset_fragmentation();

    return true;
}

bool
echo_server::on_websocket_text_frame(connection& conn, std::string_view text_data)
{
    if (text_data.empty()) {
        SPDLOG_INFO("received empty text frame from {}", conn.ip);
    } else {
        SPDLOG_INFO("received text frame from {}: {} bytes", conn.ip, text_data.size());
        if (text_data.size() <= 100) {
            SPDLOG_DEBUG("text content: '{}'", text_data);
        } else {
            SPDLOG_DEBUG("text content: '{}...' (truncated)", text_data.substr(0, 100));
        }
    }
    return true;
}

bool
echo_server::on_websocket_binary_frame(connection& conn, std::span<std::uint8_t const> payload)
{
    if (payload.empty()) {
        SPDLOG_INFO("received empty binary frame from {}", conn.ip);
    } else {
        SPDLOG_INFO("received binary frame from {}: {} bytes", conn.ip, payload.size());

        if (!payload.empty()) {
            std::string hex_preview;
            std::size_t preview_len = std::min(payload.size(), static_cast<std::size_t>(16));

            for (std::size_t i = 0; i < preview_len; ++i) {
                if (i > 0) {
                    hex_preview += " ";
                }
                hex_preview += std::format("{:02x}", payload[i]);
            }

            if (payload.size() > 16) {
                hex_preview += "...";
            }

            SPDLOG_DEBUG("binary content: {}", hex_preview);
        }
    }
    return true;
}

bool
echo_server::disconnect_and_cleanup_client(connection& conn)
{
    epoll_event event{};
    event.events = (EPOLLIN | EPOLLET);
    event.data.fd = conn.sockfd;
    if (int rv = ::epoll_ctl(epollfd_, EPOLL_CTL_DEL, conn.sockfd, &event); rv == -1) {
        SPDLOG_CRITICAL("error: epoll_ctl (EPOLL_CTL_DEL): {} {}", std::strerror(errno), errno);
        return false;
    }

    close(conn.sockfd);
    clients_.erase(conn.sockfd);
    SPDLOG_INFO("client disconnected: {}", conn);
    return true;
}


bool
echo_server::send_echo(
        connection& conn, std::span<std::uint8_t const> payload, OpCode original_frame_type)
{
    if (payload.empty()) {
        return true;
    }

    frame_generator gen;

    if (original_frame_type == OpCode::Text) {
        std::string_view text_data(reinterpret_cast<const char*>(payload.data()), payload.size());
        gen.text(text_data);
    } else {
        gen.binary(payload);
    }

    SPDLOG_DEBUG("echoing {} bytes as {}", gen.size(), original_frame_type);
    ssize_t nbytes
            = ::send(conn.sockfd, gen.data().data(), static_cast<ssize_t>(gen.size()), /*flags=*/0);
    if (nbytes == -1) {
        SPDLOG_CRITICAL("send: {}: {}", std::strerror(errno), errno);
        return false;
    }

    return true;
}

} // namespace ws
