#pragma once
#include <functional>
#include <span>
#include <string>
#include <queue>

#include <mqtt_packet.h>

#include "lwip/err.h"

#define MQTT_PORT 1883
#define MQTT_SECURE_PORT 8883

class tcp_base;
namespace mqtt {
    class client {
    public:
        client(
            std::string url,
            std::u8string username = u8"",
            std::span<uint8_t> password = {},
            std::span<uint8_t> cert = {}
        );
        ~client();

        void run();

    private:
        enum class state {
            disconnected,
            connecting,
            connected
        };
        tcp_base *m_tcp;
        // message queue for unack'd qos 1/2 sent messages
        // message queue for unack'd qos 2 recv messages
        // message queue for messages to send to the server
        std::queue<packet*> m_unacked_sends, m_unacked_recvs, m_messages;
        uint16_t m_send_quota;
        std::string m_url, m_host;
        std::u8string m_username;
        std::span<uint8_t> m_cert, m_password;
        int m_port;
        state m_state;

        bool parse_url();

        void enqueue_connect();
        void enqueue_puback(uint16_t packet_id);
        void enqueue_pubrec(uint16_t packet_id);
        void enqueue_pubrel(uint16_t packet_id);
        void enqueue_pubcomp(uint16_t packet_id);
        

        void tcp_connected_callback();
        void tcp_recv_callback();
        void tcp_closed_callback();
        void tcp_error_callback(err_t);
    };
}