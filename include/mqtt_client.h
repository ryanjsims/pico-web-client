#pragma once
#include <functional>
#include <span>
#include <string>
#include <queue>
#include <pico/time.h>
#include <vector>

#include <mqtt/packets.h>

#include "lwip/err.h"

#define MQTT_PORT 1883
#define MQTT_SECURE_PORT 8883

class tcp_base;
namespace mqtt {
    // Publish Handler function that receives the topic name, publish properties, and the publish payload
    using publish_handler_t = std::function<mqtt::reason_code(std::u8string_view, const mqtt::properties&, std::span<uint8_t>)>;
    // If this handler function is called, it is a protocol error
    const publish_handler_t NULL_SUB_HANDLER = [](std::u8string_view, const mqtt::properties&, std::span<uint8_t>){ return reason_code::ERROR_PROTOCOL; };

    class client {
    public:
        client(
            std::string url,
            std::span<uint8_t> cert = {}
        );
        ~client();

        void connect(std::u8string username, std::span<uint8_t> password);
        void connect(std::u8string username, std::u8string password);
        void connect();

        void disconnect(reason_code reason);

        void subscribe(std::u8string topic_filter, subscribe_packet::options_t options, publish_handler_t handler);
        void subscribe(std::span<std::u8string> topic_filters, std::span<subscribe_packet::options_t> options, publish_handler_t handler);

        void unsubscribe(std::u8string topic_filter);
        void unsubscribe(std::span<std::u8string> topic_filters);

        void publish(std::u8string topic, publish_packet::flags_t flags, std::span<uint8_t> data);
        void publish(std::u8string topic, publish_packet::flags_t flags, std::u8string content_type, std::span<uint8_t> data);

        bool connected();

    private:
        enum class state {
            disconnected,
            disconnecting,
            connecting,
            connected
        };

        struct subscription_t {
            std::vector<std::u8string> topic_filters;
            std::vector<uint8_t> max_qos;
            publish_handler_t handler;
            bool active;
        };

        tcp_base *m_tcp;
        // Queue containing sent packets that need to be acked by the peer
        // Only written to by send handler or when cycling through packets
        std::queue<packet*> m_unacked_sends;
        // Queue containing received packets that have not been handled
        // Only written to by tcp recv handler
        std::queue<packet*> m_recv_queue;
        // Queue containing packets to be sent to the peer
        // Only read by send handler, can be written by anyone
        std::queue<packet*> m_send_queue;
        uint16_t m_send_quota;
        uint8_t m_max_qos;
        std::string m_url, m_host;
        std::u8string m_username, m_client_id;
        std::span<uint8_t> m_cert, m_password;
        uint16_t m_current_packet_id;
        int m_port;
        state m_state;
        repeating_timer_t queue_timer;

        std::vector<subscription_t> m_subscriptions;

        bool parse_url();

        static bool queue_timer_callback(repeating_timer_t* rt);
        void handle_packet_queues();
        packet* get_next_packet();
        // Note: if packet is returned, it is removed from the unacked queue
        packet* get_unacked(packet_type type, uint16_t packet_id);
        void clear_unacked();
        void resend_reconnect();
        uint16_t generate_packet_id();

        void handle_connect(packet* p);
        void handle_connack(packet* p);
        void handle_publish(packet* p);
        void handle_puback(packet* p);
        void handle_pubrec(packet* p);
        void handle_pubrel(packet* p);
        void handle_pubcomp(packet* p);
        void handle_subscribe(packet* p);
        void handle_suback(packet* p);
        void handle_unsubscribe(packet* p);
        void handle_unsuback(packet* p);
        void handle_pingreq(packet* p);
        void handle_pingresp(packet* p);
        void handle_disconnect(packet* p);
        void handle_auth(packet* p);

        void tcp_connected_callback();
        void tcp_recv_callback();
        void tcp_closed_callback();
        void tcp_error_callback(err_t);
    };
}