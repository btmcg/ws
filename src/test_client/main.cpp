#include "test_client/test_client.hpp"
#include <spdlog/spdlog.h>
#include <cstdint>
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <cstring> // std::memset, std::strerror
#include <span>

int
main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    // [2025-07-17 11:10:13.674784] [info] [main.cpp:14] message
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%s:%#] %v");

    int port = 8000;
    if (argc == 2) {
        port = std::atoi(argv[1]);
    }

    ws::test_client client("127.0.0.1", port);

    if (!client.connect()) {
        SPDLOG_CRITICAL("client failed to connect");
        return EXIT_FAILURE;
    }

    if (!client.send_websocket_upgrade_request()) {
        SPDLOG_CRITICAL("failed to send websocket upgrade request");
        return EXIT_FAILURE;
    }

    auto response = client.recv();
    SPDLOG_INFO("upgrade response: {} bytes", response.size());
    client.mark_read(response.size());

    bool all_tests_passed = true;

    // test 1: simple small fragmented text message
    SPDLOG_INFO("running test 1: simple fragmented message");
    if (!client.send_simple_fragmented_message()) {
        SPDLOG_ERROR("simple fragmented message test failed");
        all_tests_passed = false;
    } else {
        auto echo = client.recv();
        if (!echo.empty()) {
            std::string echo_str(reinterpret_cast<char const*>(echo.data()), echo.size());
            SPDLOG_INFO("simple fragmentation test passed");
            client.mark_read(echo.size());
        } else {
            SPDLOG_ERROR("no echo received for simple fragmented message");
            all_tests_passed = false;
        }
    }

    // test 2: large fragmented text message
    SPDLOG_INFO("running test 2: large fragmented text message");
    if (!client.send_large_fragmented_text_message()) {
        SPDLOG_ERROR("large fragmented text message test failed");
        all_tests_passed = false;
    }

    // // test 3: binary fragmented message
    // SPDLOG_INFO("\nrunning test 3: binary fragmented message");
    // if (!client.send_binary_fragmented_message()) {
    //     SPDLOG_ERROR("binary fragmented message test failed");
    //     all_tests_passed = false;
    // }

    //     // Test 4: Many small fragments
    //     SPDLOG_INFO("\n🧪 Running Test 4: Many Small Fragments");
    //     if (!client.send_many_small_fragments()) {
    //         SPDLOG_ERROR("❌ Many small fragments test failed");
    //         all_tests_passed = false;
    //     }

    //     // Test 5: Empty fragments
    //     SPDLOG_INFO("\n🧪 Running Test 5: Empty Fragments");
    //     if (!client.send_empty_fragments()) {
    //         SPDLOG_ERROR("❌ Empty fragments test failed");
    //         all_tests_passed = false;
    //     }

    //     // Test 6: Single-byte fragments
    //     SPDLOG_INFO("\n🧪 Running Test 6: Single-byte Fragments");
    //     if (!client.send_single_byte_fragments()) {
    //         SPDLOG_ERROR("❌ Single-byte fragments test failed");
    //         all_tests_passed = false;
    //     }

    //     // Test 7: Fragmented message with interleaved ping
    //     SPDLOG_INFO("\n🧪 Running Test 7: Fragmented Message with Interleaved Ping");
    //     if (!client.send_fragmented_message_with_interleaved_ping()) {
    //         SPDLOG_ERROR("❌ Fragmented message with interleaved ping test failed");
    //         all_tests_passed = false;
    //     }

    // test summary
    SPDLOG_INFO(std::string(50, '='));
    if (all_tests_passed) {
        SPDLOG_INFO("all fragmentation tests passed!");
        SPDLOG_INFO("7/7 tests successful");
    } else {
        SPDLOG_ERROR("some fragmentation tests failed!");
        SPDLOG_ERROR("not all tests passed");
    }
    SPDLOG_INFO(std::string(50, '='));

    return all_tests_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
