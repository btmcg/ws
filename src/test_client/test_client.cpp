#include "test_client.hpp"
#include "ws/frame.hpp"
#include "ws/frame_generator.hpp"
#include <netdb.h> // ::getaddrinfo
#include <spdlog/spdlog.h>
#include <sys/socket.h> // ::connect, ::getsockname, ::send, ::setsockopt, ::socket
#include <sys/types.h>
#include <unistd.h>  // ::gethostname
#include <algorithm> // std::equal
#include <bit>       // std::byteswap
#include <cstdlib>   // EXIT_FAILURE, EXIT_SUCCESS
#include <cstring>   // std::memset, std::strerror


namespace ws {

test_client::test_client(std::string const& ip, int port)
        : ip_(ip)
        , port_(port)
        , sockfd_(-1)
{
    /* empty */
}

test_client::~test_client() noexcept
{
    ::close(sockfd_);
}

bool
test_client::connect()
{
    addrinfo hints{};
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE;     // wildcard ip

    // get local address
    addrinfo* result = nullptr;
    if (int rv = ::getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &result); rv != 0) {
        SPDLOG_CRITICAL("getaddrinfo: {}", std::strerror(errno));
        return false;
    }

    sockfd_ = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd_ == -1) {
        SPDLOG_CRITICAL("socket: {}", std::strerror(errno));
        return false;
    }

    if (int rv = ::connect(sockfd_, result->ai_addr, result->ai_addrlen); rv == -1) {
        SPDLOG_CRITICAL("connect: {}", std::strerror(errno));
        return false;
    }

    ::freeaddrinfo(result);
    return true;
}

std::span<std::uint8_t const>
test_client::recv()
{
    ::ssize_t nbytes = ::recv(sockfd_, buf_.write_ptr(), buf_.bytes_left(), /*flags=*/0);
    if (nbytes > 0) {
        buf_.bytes_written(nbytes);
        return std::span(buf_.read_ptr(), buf_.bytes_unread());
    } else if (nbytes == 0) {
        return {}; // connection closed
    } else {
        SPDLOG_CRITICAL("recv: {}", std::strerror(errno));
        std::abort();
    }

    return {};
}

void
test_client::mark_read(std::size_t nbytes)
{
    buf_.bytes_read(nbytes);
}

bool
test_client::send_websocket_upgrade_request()
{
    SPDLOG_DEBUG("sending websocket upgrade request");

    char hostname[HOST_NAME_MAX]; // Buffer to store the hostname
    if (int rv = ::gethostname(hostname, HOST_NAME_MAX); rv != 0) {
        SPDLOG_CRITICAL("gethostname: {}", std::strerror(errno));
        return false;
    }

    sockaddr_in sa;
    socklen_t socklen = static_cast<socklen_t>(sizeof(sa));
    if (int rv = ::getsockname(sockfd_, reinterpret_cast<sockaddr*>(&sa), &socklen); rv == -1) {
        SPDLOG_CRITICAL("recv: {}", std::strerror(errno));
        return false;
    }

    int const local_port = std::byteswap(sa.sin_port);

    frame_generator frame_gen;

    std::string const websocket_key = frame_gen.generate_websocket_key();
    std::string const request = std::format("GET / HTTP/1.1\r\n"
                                            "Host: {}:{}\r\n"
                                            "Upgrade: websocket\r\n"
                                            "Connection: Upgrade\r\n"
                                            "Sec-WebSocket-Key: {}\r\n"
                                            "Sec-WebSocket-Version: 13\r\n"
                                            "\r\n",
            hostname, local_port, websocket_key);

    if (ssize_t nbytes = ::send(sockfd_, request.data(), request.size(), /*flags=*/0);
            nbytes != static_cast<ssize_t>(request.size())) {
        SPDLOG_CRITICAL("send: {}", std::strerror(errno));
        return false;
    }

    return true;
}

bool
test_client::send_simple_fragmented_message()
{
    SPDLOG_INFO("=== testing simple fragmented message ===");

    ssize_t nbytes = -1;

    std::string part1 = "hello";
    std::string part2 = " world!";

    // first fragment (text frame, FIN=false)
    auto frame1 = frame_generator{}.text(part1, /*fin=*/false, /*mask=*/true);
    if (nbytes = ::send(sockfd_, frame1.data().data(), frame1.size(), /*flags=*/0);
            nbytes != static_cast<ssize_t>(frame1.size())) {
        SPDLOG_CRITICAL("send: {}", std::strerror(errno));
        return false;
    }
    SPDLOG_DEBUG("send_simple_fragmented_message: sent fragment 1: {} bytes", nbytes);

    // second fragment (continuation frame, FIN=true)
    auto frame2 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(part2.data()), part2.size()),
            /*fin=*/true, /*mask=*/true);
    if (nbytes = ::send(sockfd_, frame2.data().data(), frame2.size(), /*flags=*/0);
            nbytes != static_cast<ssize_t>(frame2.size())) {
        SPDLOG_CRITICAL("send: {}", std::strerror(errno));
        return false;
    }

    SPDLOG_DEBUG("send_simple_fragmented_message: sent fragment 2: {} bytes", nbytes);
    return true;
}

bool
test_client::send_large_fragmented_text_message()
{
    SPDLOG_INFO("=== testing large fragmented text message ===");

    // create a large message (10KB)
    std::string large_text = generate_large_text(10240);
    std::string part1 = large_text.substr(0, 3000);
    std::string part2 = large_text.substr(3000, 4000);
    std::string part3 = large_text.substr(7000);

    SPDLOG_DEBUG("splitting message: total={}, part1={}, part2={}, part3={}", large_text.size(),
            part1.size(), part2.size(), part3.size());

    // first fragment (text frame, FIN=false)
    auto frame1 = frame_generator{}.text(part1, /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame1.data().data(), frame1.size(), 0)
            != static_cast<ssize_t>(frame1.size())) {
        SPDLOG_ERROR("failed to send fragment 1");
        return false;
    }
    SPDLOG_DEBUG("sent fragment 1: {} bytes", frame1.size());

    // second fragment (continuation frame, FIN=false)
    auto frame2 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(part2.data()), part2.size()),
            /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame2.data().data(), frame2.size(), 0)
            != static_cast<ssize_t>(frame2.size())) {
        SPDLOG_ERROR("failed to send fragment 2");
        return false;
    }
    SPDLOG_DEBUG("sent fragment 2: {} bytes", frame2.size());

    // third fragment (continuation frame, FIN=true)
    auto frame3 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(part3.data()), part3.size()),
            /*fin=*/true, /*mask=*/true);
    if (::send(sockfd_, frame3.data().data(), frame3.size(), 0)
            != static_cast<ssize_t>(frame3.size())) {
        SPDLOG_ERROR("failed to send fragment 3");
        return false;
    }
    SPDLOG_DEBUG("sent fragment 3: {} bytes", frame3.size());

    return expect_echo_response(large_text);
}

bool
test_client::send_binary_fragmented_message()
{
    SPDLOG_INFO("=== testing binary fragmented message ===");

    std::vector<std::uint8_t> binary_data = generate_binary_data(1024);

    // split into 3 parts
    std::vector<std::uint8_t> part1(binary_data.begin(), binary_data.begin() + 300);
    std::vector<std::uint8_t> part2(binary_data.begin() + 300, binary_data.begin() + 700);
    std::vector<std::uint8_t> part3(binary_data.begin() + 700, binary_data.end());

    // first fragment (binary frame, FIN=false)
    auto frame1 = frame_generator{}.binary(
            std::span<std::uint8_t const>(part1), /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame1.data().data(), frame1.size(), 0)
            != static_cast<ssize_t>(frame1.size())) {
        SPDLOG_ERROR("failed to send binary fragment 1");
        return false;
    }

    // second fragment (continuation frame, FIN=false)
    auto frame2 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(part2), /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame2.data().data(), frame2.size(), 0)
            != static_cast<ssize_t>(frame2.size())) {
        SPDLOG_ERROR("failed to send binary fragment 2");
        return false;
    }

    // third fragment (continuation frame, FIN=true)
    auto frame3 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(part3), /*fin=*/true, /*mask=*/true);
    if (::send(sockfd_, frame3.data().data(), frame3.size(), 0)
            != static_cast<ssize_t>(frame3.size())) {
        SPDLOG_ERROR("failed to send binary fragment 3");
        return false;
    }

    return expect_binary_echo_response(binary_data);
}

bool
test_client::send_many_small_fragments()
{
    SPDLOG_INFO("=== testing many small fragments ===");

    std::string complete_message;
    std::string alphabet = "abcdefghijklmnopqrstuvwxyz";

    // first fragment (text frame, FIN=false)
    auto frame1 = frame_generator{}.text("start:", /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame1.data().data(), frame1.size(), 0)
            != static_cast<ssize_t>(frame1.size())) {
        return false;
    }
    complete_message += "start:";

    // send 26 small continuation fragments (one for each letter)
    for (std::size_t i = 0; i < 25; ++i) {
        std::string letter(1, alphabet[i]);
        auto frame = frame_generator{}.continuation(
                std::span<std::uint8_t const>(
                        reinterpret_cast<std::uint8_t const*>(letter.data()), 1),
                /*fin=*/false, /*mask=*/true);
        if (::send(sockfd_, frame.data().data(), frame.size(), 0)
                != static_cast<ssize_t>(frame.size())) {
            SPDLOG_ERROR("failed to send fragment for letter {}", letter);
            return false;
        }
        complete_message += letter;
    }

    // final fragment (continuation frame, FIN=true)
    std::string final_letter(1, alphabet[25]);
    auto final_frame = frame_generator{}.continuation(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(final_letter.data()), 1),
            /*fin=*/true, /*mask=*/true);
    if (::send(sockfd_, final_frame.data().data(), final_frame.size(), 0)
            != static_cast<ssize_t>(final_frame.size())) {
        return false;
    }
    complete_message += final_letter;

    SPDLOG_DEBUG("complete fragmented message: '{}'", complete_message);
    return expect_echo_response(complete_message);
}

bool
test_client::send_mixed_control_and_fragmented_message()
{
    // This is the same as send_fragmented_message_with_interleaved_ping
    return send_fragmented_message_with_interleaved_ping();
}

bool
test_client::send_empty_fragments()
{
    SPDLOG_INFO("=== testing empty fragments ===");

    // first fragment with actual content
    auto frame1 = frame_generator{}.text("hello", /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame1.data().data(), frame1.size(), 0)
            != static_cast<ssize_t>(frame1.size())) {
        return false;
    }

    // empty continuation fragment
    auto frame2 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(), /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame2.data().data(), frame2.size(), 0)
            != static_cast<ssize_t>(frame2.size())) {
        return false;
    }

    // another fragment with content
    std::string part3 = " world";
    auto frame3 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(part3.data()), part3.size()),
            /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame3.data().data(), frame3.size(), 0)
            != static_cast<ssize_t>(frame3.size())) {
        return false;
    }

    // final empty fragment
    auto frame4 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(), /*fin=*/true, /*mask=*/true);
    if (::send(sockfd_, frame4.data().data(), frame4.size(), 0)
            != static_cast<ssize_t>(frame4.size())) {
        return false;
    }

    return expect_echo_response("hello world");
}

bool
test_client::send_single_byte_fragments()
{
    SPDLOG_INFO("=== testing single-byte fragments ===");

    std::string message = "BYTE";

    // send each byte as a separate fragment
    for (std::size_t i = 0; i < message.size(); ++i) {
        bool is_final = (i == message.size() - 1);
        std::string byte_str(1, message[i]);

        if (i == 0) {
            // first fragment (text frame)
            auto frame = frame_generator{}.text(byte_str, /*fin=*/is_final, /*mask=*/true);
            if (::send(sockfd_, frame.data().data(), frame.size(), 0)
                    != static_cast<ssize_t>(frame.size())) {
                return false;
            }
        } else {
            // continuation fragments
            auto frame = frame_generator{}.continuation(
                    std::span<std::uint8_t const>(
                            reinterpret_cast<std::uint8_t const*>(byte_str.data()), 1),
                    /*fin=*/is_final, /*mask=*/true);
            if (::send(sockfd_, frame.data().data(), frame.size(), 0)
                    != static_cast<ssize_t>(frame.size())) {
                return false;
            }
        }
    }

    return expect_echo_response(message);
}

bool
test_client::send_fragmented_message_with_interleaved_ping()
{
    SPDLOG_INFO("=== testing fragmented message with interleaved ping ===");

    // start fragmented message
    auto frame1 = frame_generator{}.text("first", /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame1.data().data(), frame1.size(), 0)
            != static_cast<ssize_t>(frame1.size())) {
        return false;
    }

    // send a ping frame (should be handled independently)
    if (!send_ping("ping_during_fragmentation")) {
        return false;
    }

    // continue fragmented message
    std::string part2 = " second";
    auto frame2 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(part2.data()), part2.size()),
            /*fin=*/false, /*mask=*/true);
    if (::send(sockfd_, frame2.data().data(), frame2.size(), 0)
            != static_cast<ssize_t>(frame2.size())) {
        return false;
    }

    // send another ping
    if (!send_ping("another_ping")) {
        return false;
    }

    // finish fragmented message
    std::string part3 = " third";
    auto frame3 = frame_generator{}.continuation(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(part3.data()), part3.size()),
            /*fin=*/true, /*mask=*/true);
    if (::send(sockfd_, frame3.data().data(), frame3.size(), 0)
            != static_cast<ssize_t>(frame3.size())) {
        return false;
    }

    // we should receive pong responses and the echo
    return expect_echo_response("first second third");
}

bool
test_client::send_ping(std::string const& payload)
{
    auto ping_frame = frame_generator{}.ping(
            std::span<std::uint8_t const>(
                    reinterpret_cast<std::uint8_t const*>(payload.data()), payload.size()),
            /*mask=*/true);

    if (::send(sockfd_, ping_frame.data().data(), ping_frame.size(), 0)
            != static_cast<ssize_t>(ping_frame.size())) {
        SPDLOG_ERROR("failed to send ping");
        return false;
    }

    SPDLOG_DEBUG("sent ping with payload: '{}'", payload);
    return true;
}

bool
test_client::expect_echo_response(std::string const& expected_text)
{
    // We may receive multiple frames before getting the echo response
    // due to interleaved control frames (pong responses to ping).
    while (true) {
        // Check if we already have data in the buffer
        if (buf_.bytes_unread() == 0) {
            auto response = recv();
            if (response.empty()) {
                SPDLOG_ERROR("no echo response received (connection closed or error)");
                return false;
            }
            SPDLOG_DEBUG("received {} bytes from server", response.size());
        }

        // Process all complete frames in the buffer
        while (buf_.bytes_unread() > 0) {
            SPDLOG_DEBUG("processing buffer with {} bytes unread", buf_.bytes_unread());

            frame frame;
            ParseResult result = frame.parse_from_buffer(buf_.read_ptr(), buf_.bytes_unread());

            if (result == ParseResult::NeedMoreData) {
                SPDLOG_DEBUG("need more data for complete frame, breaking to recv more");
                break; // Need to receive more data
            }

            if (result != ParseResult::Success) {
                SPDLOG_ERROR("failed to parse frame: result={}", static_cast<int>(result));
                return false;
            }

            SPDLOG_DEBUG("parsed frame: opcode={}, fin={}, payload_len={}",
                    static_cast<int>(frame.op_code()), frame.fin(), frame.payload_len());

            switch (frame.op_code()) {
                case OpCode::Text: {
                    // this is our echo response
                    auto text_payload = frame.get_text_payload();
                    if (!text_payload) {
                        SPDLOG_ERROR("echo response is not a valid text frame");
                        buf_.bytes_read(frame.total_size());
                        return false;
                    }

                    std::string received_text = text_payload.value();
                    SPDLOG_DEBUG("received text frame: '{}'", received_text);

                    if (received_text != expected_text) {
                        SPDLOG_ERROR("echo mismatch. expected:\n[{}]\nreceived:\n[{}]",
                                expected_text, received_text);
                        buf_.bytes_read(frame.total_size());
                        return false;
                    }

                    SPDLOG_INFO(
                            "echo response matches expected text ({} bytes)", expected_text.size());
                    buf_.bytes_read(frame.total_size());
                    return true;
                }

                case OpCode::Pong: {
                    auto payload = frame.get_payload_data();
                    std::string pong_payload(
                            reinterpret_cast<const char*>(payload.data()), payload.size());
                    SPDLOG_DEBUG("received pong response with payload: '{}'", pong_payload);
                    buf_.bytes_read(frame.total_size());
                    // Continue processing more frames in buffer
                    break;
                }

                case OpCode::Ping: {
                    // unexpected ping from server - we should respond but this is unusual
                    SPDLOG_WARN("received unexpected ping from server during echo test");
                    buf_.bytes_read(frame.total_size());
                    // continue processing more frames in buffer
                    break;
                }

                case OpCode::Close: {
                    SPDLOG_ERROR("received close frame instead of echo response");
                    return false;
                }

                case OpCode::Binary:
                case OpCode::Continuation:
                default: {
                    SPDLOG_ERROR("received unexpected frame type: {}",
                            static_cast<int>(frame.op_code()));
                    buf_.bytes_read(frame.total_size());
                    return false;
                }
            }
        }

        // If we get here, we've processed all frames in the buffer but
        // haven't found our echo yet. Continue the outer loop to recv()
        // more data.
        SPDLOG_DEBUG("processed all frames in buffer, waiting for more data...");
    }
}

bool
test_client::expect_binary_echo_response(std::vector<std::uint8_t> const& expected_data)
{
    while (true) {
        auto response = recv();
        if (response.empty()) {
            SPDLOG_ERROR("no binary echo response received");
            return false;
        }

        frame frame;
        ParseResult result = frame.parse_from_buffer(response.data(), response.size());

        if (result != ParseResult::Success) {
            SPDLOG_ERROR("failed to parse binary frame: result={}", static_cast<int>(result));
            return false;
        }

        switch (frame.op_code()) {
            case OpCode::Binary: {
                // this is our echo response
                auto payload = frame.get_payload_data();

                if (payload.size() != expected_data.size()) {
                    SPDLOG_ERROR("binary echo size mismatch. expected: {}, received: {}",
                            expected_data.size(), payload.size());
                    return false;
                }

                if (!std::equal(payload.begin(), payload.end(), expected_data.begin())) {
                    SPDLOG_ERROR("binary echo content mismatch");
                    return false;
                }

                SPDLOG_INFO("binary echo response matches expected data ({} bytes)",
                        expected_data.size());
                mark_read(response.size());
                return true;
            }

            case OpCode::Pong: {
                auto payload = frame.get_payload_data();
                std::string pong_payload(
                        reinterpret_cast<const char*>(payload.data()), payload.size());
                SPDLOG_DEBUG("received pong response with payload: '{}'", pong_payload);
                mark_read(response.size());
                // continue loop to wait for echo response
                break;
            }

            case OpCode::Ping: {
                // unexpected ping from server
                SPDLOG_WARN("received unexpected ping from server during binary echo test");
                mark_read(response.size());
                break;
            }

            case OpCode::Close: {
                SPDLOG_ERROR("received close frame instead of binary echo response");
                return false;
            }

            case OpCode::Continuation:
            case OpCode::Text:
            default: {
                SPDLOG_ERROR(
                        "received unexpected frame type: {}", static_cast<int>(frame.op_code()));
                return false;
            }
        }
    }
}
std::string
test_client::generate_large_text(std::size_t size)
{
    std::string result;
    result.reserve(size);

    std::string pattern = "The quick brown fox jumps over the lazy dog. ";
    while (result.size() < size) {
        std::size_t remaining = size - result.size();
        if (remaining >= pattern.size()) {
            result += pattern;
        } else {
            result += pattern.substr(0, remaining);
        }
    }

    return result;
}

std::vector<std::uint8_t>
test_client::generate_binary_data(std::size_t size)
{
    std::vector<std::uint8_t> data;
    data.reserve(size);

    for (std::size_t i = 0; i < size; ++i) {
        data.push_back(static_cast<std::uint8_t>(i % 256));
    }

    return data;
}

} // namespace ws
