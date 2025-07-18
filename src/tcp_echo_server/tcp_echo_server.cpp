#include "tcp_echo_server.hpp"
#include "../base64_codec.hpp"
#include "../sha1.hpp"
#include "../str_utils.hpp"
#include "../websocket_frame.hpp"
#include <arpa/inet.h> // ::inet_ntop
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>  // ::close
#include <algorithm> // std::find_if
#include <cstdlib>   // std::abort
#include <cstring>
#include <print>
#include <unordered_map>
#include <vector>

namespace ws {

static constexpr std::string_view GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

tcp_echo_server::tcp_echo_server(int port)
        : sock_(-1)
        , port_(port)
        , header_fields_()
        , clients_()
        , accept_key_()
{
    SPDLOG_DEBUG("tcp_echo_server instantiated");
    sock_ = ::socket(PF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        SPDLOG_CRITICAL("socket: {}: {}", std::strerror(errno), errno);
    }

    sockaddr_in addr{.sin_family = AF_INET,
            .sin_port = std::byteswap(std::uint16_t(port)),
            .sin_addr = {INADDR_ANY},
            .sin_zero = 0};

    if (::bind(sock_, reinterpret_cast<sockaddr const*>(&addr), sizeof(addr)) == -1) {
        SPDLOG_CRITICAL("bind: {}: {}", std::strerror(errno), errno);
        std::abort();
    }
}

tcp_echo_server::~tcp_echo_server()
{
    ::close(sock_);
}

bool
tcp_echo_server::listen()
{
    if (int rv = ::listen(sock_, 5); rv == -1) {
        SPDLOG_CRITICAL("listen: {}: {}", std::strerror(errno), errno);
        return false;
    }

    SPDLOG_INFO("tcp_echo_server listening on port {}...", port_);

    while (true) {
        sockaddr_storage their_addr;
        socklen_t addr_size = sizeof(their_addr);
        int client_fd = ::accept(sock_, reinterpret_cast<sockaddr*>(&their_addr), &addr_size);
        if (their_addr.ss_family != AF_INET) {
            SPDLOG_CRITICAL("only ipv4 implemented");
            std::abort();
        }
        auto const* sin = reinterpret_cast<sockaddr_in const*>(&their_addr);
        clients_.emplace(sin->sin_addr.s_addr, client_fd);
        char ip_storage[INET_ADDRSTRLEN];
        char const* ip = nullptr;
        if (ip = ::inet_ntop(AF_INET, &(sin->sin_addr), ip_storage, INET_ADDRSTRLEN);
                ip == nullptr) {
            SPDLOG_CRITICAL("inet_ntop: {}: {}", std::strerror(errno), errno);
            std::abort();
        }

        SPDLOG_INFO("client [{}:{}] connected, fd={}", ip, sin->sin_port, client_fd);

        std::array<char, 4096> buf{};
        ssize_t const bytes_read = ::recv(client_fd, buf.data(), buf.size(), 0);
        if (bytes_read == 0) {
            ::close(sock_);
            continue;
        }

        std::string str(buf.data(), static_cast<std::size_t>(bytes_read));
        SPDLOG_DEBUG("{}", str);

        SPDLOG_DEBUG("- - -");
        if (!parse_request(str)) {
            SPDLOG_ERROR("parse_request failed");
            return false;
        }
        SPDLOG_DEBUG("- - -");

        if (!send_response(client_fd)) {
            SPDLOG_ERROR("failed to send response");
            return false;
        } else {
            SPDLOG_INFO("response sent");
        }
    }

    return true;
}

bool
tcp_echo_server::parse_request(std::string const& req)
{
    std::string method;

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
        header_fields_.emplace(to_lower(header_key), header_val);

        pos = newline_pos + 2;
    }

    if (!validate_header_fields()) {
        SPDLOG_ERROR("header fields validation failed");
        return false;
    }

    return true;
}

bool
tcp_echo_server::validate_request_method_uri_and_version(std::string const& method_and_ver)
{
    // According to RFC 2616, section 5.1.1
    // The Method token indicates the method to be performed on the resource identified by the
    // Request-URI. The method is case-sensitive.
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
tcp_echo_server::validate_header_fields()
{
    // According to RFC 7230, section 3.2:
    // Each header field consists of a case-insensitive field name
    // followed by a colon (":"), optional leading whitespace, the field
    // value, and optional trailing whitespace.
    for (auto& [key, val] : header_fields_) {
        SPDLOG_DEBUG("header_fields key={}, val={}", key, val);
    }

    // Upgrade
    {
        constexpr char key[] = "upgrade";
        if (!header_fields_.contains(key)) {
            SPDLOG_CRITICAL("missing '{}' field", key);
            return false;
        }
        auto const& val = to_lower(header_fields_[key]);
        if (val != "websocket") {
            SPDLOG_CRITICAL("invalid '{}' value: [{}]", key, val);
            return false;
        }
    }

    // Connection
    {
        constexpr char key[] = "connection";
        if (!header_fields_.contains(key)) {
            SPDLOG_CRITICAL("missing '{}' field", key);
            return false;
        }
        auto const& val = header_fields_[key];
        if (!val.contains("Upgrade")) {
            SPDLOG_CRITICAL("invalid '{}' value: [{}]", key, val);
            return false;
        }
    }

    // Sec-Websocket-Version
    {
        constexpr char key[] = "sec-websocket-version";
        if (!header_fields_.contains(key)) {
            SPDLOG_CRITICAL("missing '{}' field", key);
            return false;
        }
        auto const& val = header_fields_[key];
        if (val.empty()) {
            SPDLOG_CRITICAL("invalid '{}' value: [{}]", key, val);
            return false;
        }
    }

    // Sec-Websocket-Key
    {
        constexpr char key[] = "sec-websocket-key";
        if (!header_fields_.contains(key)) {
            SPDLOG_CRITICAL("missing '{}' field", key);
            return false;
        }
        auto const& val = header_fields_[key];
        if (val.empty()) {
            SPDLOG_CRITICAL("invalid '{}' value: [{}]", key, val);
            return false;
        }

        accept_key_ = generate_accept_key(val);
    }

    return true;
}

std::string
tcp_echo_server::generate_accept_key(std::string const& key)
{
    SPDLOG_INFO("key={}", key);
    std::string const concat = key + std::string(GUID);
    SPDLOG_INFO("concat={}", concat);

    // Get the raw SHA-1 hash bytes (not hex string!)
    auto const sha1_digest = sha1::hash(concat);
    SPDLOG_INFO("sha1_digest raw bytes computed");

    // Base64-encode the raw bytes directly
    std::string_view raw_bytes(
            reinterpret_cast<const char*>(sha1_digest.data()), sha1_digest.size());
    std::string b64_hash = to_base64(raw_bytes);
    SPDLOG_INFO("b64_hash={}", b64_hash);
    return b64_hash;
}

bool
tcp_echo_server::send_response(int client_fd)
{
    std::string response;
    response += "HTTP/1.1 101 Switching Protocols\r\n";
    response += "Upgrade: websocket\r\n";
    response += "Connection: Upgrade\r\n";
    response += "Sec-WebSocket-Accept: " + accept_key_ + "\r\n\r\n";
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
