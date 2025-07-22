#include "echo_server.hpp"
#include "../base64_codec.hpp"
#include "../sha1.hpp"
#include "../str_utils.hpp"
#include "../websocket_frame.hpp"
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
#include <unordered_map>
#include <vector>

namespace ws {
namespace {
    static constexpr int ListenBacklog = 10;     ///< max num of pending connections
    static constexpr int EpollMaxEvents = 20;    ///< max num of pending epoll events
    static constexpr int EpollTimeoutMsecs = 10; ///< num of milliseconds to block on epoll_wait
    static constexpr int IncomingBufferSizeBytes = 4096; ///< size of recv buffer
    static constexpr std::string_view MagicGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
} // namespace


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
    hints.ai_protocol = 0;
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;

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

    if (int rv = ::bind(sockfd_, result->ai_addr, result->ai_addrlen); rv == -1) {
        throw std::runtime_error(std::string("bind: ") + std::strerror(errno));
    }

    ::freeaddrinfo(result);

    // allow for socket reuse
    int const yes = 1;
    if (int rv = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); rv == -1) {
        throw std::runtime_error(std::string("setsockopt (SO_REUSEADDR): ") + std::strerror(errno));
    }
    if (int rv = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)); rv == -1) {
        throw std::runtime_error(std::string("setsockopt (SO_REUSEPORT): ") + std::strerror(errno));
    }

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
                bool const status = on_incoming_data(events[i].data.fd); // NOLINT
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

    auto const* sin = reinterpret_cast<sockaddr_in const*>(&their_addr);
    char ip_storage[INET_ADDRSTRLEN];
    char const* ip = nullptr;
    if (ip = ::inet_ntop(AF_INET, &(sin->sin_addr), ip_storage, INET_ADDRSTRLEN); ip == nullptr) {
        SPDLOG_CRITICAL("inet_ntop: {}: {}", std::strerror(errno), errno);
        std::abort();
    }

    SPDLOG_INFO("client [{}:{}] connected on socket fd={}", ip, sin->sin_port, accepted_sock);

    // successfully connected. create a client entry, keyed by the
    // socket fd

    clients_.emplace(accepted_sock, connection{});

    // add the new fd to epoll
    epoll_event event{};
    event.events = (EPOLLIN | EPOLLET);
    event.data.fd = accepted_sock;
    if (int rv = ::epoll_ctl(epollfd_, EPOLL_CTL_ADD, accepted_sock, &event); rv == -1) {
        SPDLOG_CRITICAL("error: epoll_ctl (EPOLL_CTL_ADD): {} {}", std::strerror(errno), errno);
        return false;
    }

    return true;
}

bool
echo_server::on_incoming_data(int fd) noexcept
{
    char buf[IncomingBufferSizeBytes];

    ssize_t const bytes_recvd = ::recv(fd, &buf, sizeof(buf), 0);
    if (bytes_recvd == -1) {
        SPDLOG_CRITICAL("error: epoll_ctl (EPOLL_CTL_ADD): {} {}", std::strerror(errno), errno);
        return false;
    }

    // client disconnected
    if (bytes_recvd == 0) {
        SPDLOG_INFO("client on fd {} disconnected", fd);

        auto itr = clients_.find(fd);
        if (itr == clients_.end()) {
            SPDLOG_CRITICAL("failed to find client entry for fd={}", fd);
            std::abort();
        }
        clients_.erase(itr);

        epoll_event event{};
        event.events = (EPOLLIN | EPOLLET);
        event.data.fd = fd;
        if (int rv = ::epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &event); rv == -1) {
            SPDLOG_CRITICAL("error: epoll_ctl (EPOLL_CTL_DEL): {} {}", std::strerror(errno), errno);
            return false;
        }

        assert(::close(fd) != -1);
        return true;
    }

    std::string str(buf, static_cast<std::size_t>(bytes_recvd));
    SPDLOG_DEBUG("{}", str);

    if (str.starts_with("GET")) {
        if (!on_http_request(fd, str)) {
            SPDLOG_ERROR("on_http_request failed");
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
echo_server::on_http_request(int fd, std::string const& req) const noexcept
{
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

        std::string header_key = req.substr(pos, colon_pos - pos);
        std::string header_val = req.substr(colon_pos + 1, newline_pos - colon_pos - 1);

        trim(header_key);
        trim(header_val);
        header_fields.emplace(to_lower(header_key), header_val);

        pos = newline_pos + 2;
    }

    if (header_fields.contains("upgrade")) {
        if (!on_websocket_upgrade_request(fd, header_fields)) {
            SPDLOG_ERROR("invalid websocket upgrade request");
            return false;
        }
    } else {
        // TODO
    }

    return true;
}

bool
echo_server::on_websocket_upgrade_request(
        int fd, std::unordered_map<std::string, std::string> const& header_fields) const noexcept
{
    if (!validate_header_fields(header_fields)) {
        SPDLOG_ERROR("header fields validation failed");
        return false;
    }

    auto itr = header_fields.find("sec-websocket-key");
    assert(itr != header_fields.end());

    if (!send_websocket_accept(fd, itr->second)) {
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
        int client_fd, std::string const& sec_websocket_key) const noexcept
{
    std::string accept_key = generate_accept_key(sec_websocket_key);
    std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Accept: "
            + accept_key + "\r\n\r\n";
    SPDLOG_DEBUG("response=\n{}", response);

    SPDLOG_DEBUG("sending {} bytes", response.size());
    ssize_t nbytes = ::send(client_fd, response.c_str(), static_cast<ssize_t>(response.size()), 0);
    if (nbytes == -1) {
        SPDLOG_CRITICAL("send: {}: {}", std::strerror(errno), errno);
        return false;
    }

    return true;
}

} // namespace ws
