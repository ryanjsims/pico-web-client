#pragma once
#include <functional>
#include <span>
#include <string>
#include <queue>
#include <vector>

#include <pico/time.h>
#include <pico/sync.h>

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

        void connect();
        void connect(std::u8string username, std::u8string password);
        void connect(std::u8string username, std::u8string password, uint16_t keep_alive);
        void connect(std::u8string username, std::span<uint8_t> password, uint16_t keep_alive);
        void connect(std::u8string username, std::span<uint8_t> password, uint16_t keep_alive, std::u8string will_topic, std::span<uint8_t> will_payload, uint8_t will_qos, bool will_retain, mqtt::properties properties, mqtt::properties will_properties);

        void on_connect(std::function<void()> user_connect_callback) {
            m_user_connected = user_connect_callback;
        }

        void disconnect(reason_code reason);

        /**
         * Blocks until connected if client is connecting
         * Handler is: 
         * ____ mqtt::reason_code (
         * ________ std::u8string_view,
         * ________ const mqtt::properties&,
         * ________ std::span<uint8_t>
         * ____ )
         */
        void subscribe(std::u8string topic_filter, subscribe_packet::options_t options, publish_handler_t handler);
        /**
         * Blocks until connected if client is connecting
         * Handler is: 
         * ____ mqtt::reason_code (
         * ________ std::u8string_view,
         * ________ const mqtt::properties&,
         * ________ std::span<uint8_t>
         * ____ )
         */
        void subscribe(std::span<std::u8string> topic_filters, std::span<subscribe_packet::options_t> options, publish_handler_t handler);

        void unsubscribe(std::u8string topic_filter);
        void unsubscribe(std::span<std::u8string> topic_filters);

        // Blocks until connected if client is connecting
        void publish(std::u8string topic, publish_packet::flags_t flags, std::span<uint8_t> data);
        // Blocks until connected if client is connecting
        void publish(std::u8string topic, publish_packet::flags_t flags, std::u8string content_type, std::span<uint8_t> data);

        bool connected();
        bool disconnected();

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
        std::queue<std::pair<uint32_t, packet*>> m_unacked_sends;
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
        uint16_t m_keep_alive;
        uint32_t m_last_send_time, m_last_recv_time;
        int m_port;
        state m_state;
        repeating_timer_t queue_timer;
        critical_section_t generate_id_section;

        std::vector<subscription_t> m_subscriptions;
        std::function<void()> m_user_connected;

        bool parse_url();

        static bool queue_timer_callback(repeating_timer_t* rt);
        void handle_packet_queues();

        void send_packet(packet*);
        void recv_packet();
        void resend_first_unacked();

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
        void tcp_send_callback(uint16_t);
        void tcp_closed_callback();
        void tcp_error_callback(err_t);
    };
}