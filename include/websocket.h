#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <cstring>
#include <functional>

#include "circular_buffer.h"
#include "tcp_base.h"
#include "logger.h"
class eio_client;

extern "C" {
    int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen);
}

namespace ws {
    constexpr uint8_t final_fragment = 0x80;
    constexpr uint8_t additional_fragment = 0x00;
    constexpr uint8_t masked = 0x80;
    constexpr uint8_t has_length_16 = 0x7E;
    constexpr uint8_t has_length_64 = 0x7F;
    enum class opcodes : uint8_t {
        continuation,
        text,
        binary,
        close = 8u,
        ping,
        pong
    };

    class websocket {
    public:
        friend class ::eio_client;
        websocket(tcp_base *socket);
        ~websocket();

        // Needs up to 14 add'l bytes to encode packet
        bool write_text(std::span<uint8_t> data);
        bool write_binary(std::span<uint8_t> data);

        void close(err_t reason = ERR_CLSD);

        size_t read(std::span<uint8_t> data);
        uint32_t received_packet_size();

        bool connected();

        void on_receive(std::function<void()> callback);
        void on_poll(uint8_t interval_seconds, std::function<void()> callback);
        void on_closed(std::function<void(err_t)> callback);

    private:
        tcp_base *tcp;
        std::function<void()> user_receive_callback, user_poll_callback;
        std::function<void(err_t)> user_close_callback;
        uint32_t packet_size;

        void mask(std::span<uint8_t> data, uint32_t masking_key);
        void tcp_recv_callback();
        void tcp_poll_callback();
        void tcp_close_callback(err_t reason);
        bool write_frame(std::span<uint8_t> data, opcodes opcode);
    };
}