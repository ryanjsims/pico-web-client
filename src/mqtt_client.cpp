#include <mqtt_client.h>

#include <logger.h>
#include <tcp_client.h>
#include <tcp_tls_client.h>

#include "LUrlParser.h"

namespace mqtt {
    uint16_t get_u16(std::span<uint8_t> data) {
        return uint16_t{(data[0] << 8) | data[1]};
    }
}

mqtt::client::client(
    std::string url,
    std::u8string username,
    std::span<uint8_t> password,
    std::span<uint8_t> cert
)
    : m_url(url)
    , m_host("")
    , m_port(-1)
    , m_cert(cert)
    , m_tcp(nullptr)
    , m_state(mqtt::client::state::disconnected)
    , m_username(username)
    , m_password(password)
{
    parse_url();
}

bool mqtt::client::parse_url() {
    debug("mqtt_client::parse_url '%.*s'\n", m_url.size(), m_url.data());
    LUrlParser::ParseURL url_parser = LUrlParser::ParseURL::parseURL(m_url);
    if(!url_parser.isValid()) {
        error("Invalid URL: %.*s\n", m_url.size(), m_url.data());
        trace1("mqtt_client::parse_url exited\n");
        return false;
    }

    m_host = url_parser.host_;
    debug("mqtt_client::parse_url got host %.*s", m_host.size(), m_host.data());
    if(url_parser.port_.size() > 0) {
        url_parser.getPort(&m_port);
        debug_cont(":%d", m_port);
    }
    debug_cont1("\n");

    if((url_parser.scheme_ == "mqtts")) {
        if(m_tcp && !m_tcp->secure()) {
            delete m_tcp;
            m_tcp = nullptr;
        }

        if(m_tcp == nullptr) {
            debug1("mqtt_client::parse_url creating new tcp_tls_client\n");
            m_tcp = new tcp_tls_client(m_cert);
        }
        if(m_port == -1) {
            m_port = MQTT_SECURE_PORT;
        }
    } else {
        if(m_tcp && m_tcp->secure()) {
            delete m_tcp;
            m_tcp = nullptr;
        }

        if(m_tcp == nullptr) {
            debug1("mqtt_client::parse_url creating new tcp_client\n");
            m_tcp = new tcp_client();
        }
        if(m_port == -1) {
            m_port = MQTT_PORT;
        }
    }
    trace1("mqtt_client::parse_url exited\n");
    return true;
}

void mqtt::client::tcp_connected_callback() {
    debug1("mqtt::client::tcp_connected_callback\n");
    m_state = mqtt::client::state::connected;
    enqueue_connect();
}

void mqtt::client::tcp_recv_callback() {
    debug1("mqtt::client::tcp_recv_callback\n");

}

void mqtt::client::enqueue_connect() {
    packet *connect_ptr = new packet(mqtt::packet_type::CONNECT);
    packet& connect = *connect_ptr;
    connect += u8"MQTT";
    connect += uint8_t{5};
    // flags
    connect += uint8_t{((int)(m_username.size() > 0) << 7) | ((int)(m_password.size() > 0) << 6)};
    // keep alive
    connect += uint16_t{0};
    // Empty properties
    connect += mqtt::properties();
    // Client id
    connect += u8"";
    // Add username and password if present
    if(m_username.size() > 0) {
        connect += m_username;
    }
    if(m_password.size() > 0) {
        connect += m_password;
    }
    m_messages.push(connect_ptr);
}

void mqtt::client::enqueue_puback(uint16_t packet_id) {
    packet *puback_ptr = new packet(mqtt::packet_type::PUBACK);
    packet& puback = *puback_ptr;
    // Set packet id
    puback += packet_id;
    // For now assume all packets are successful (reason code and properties are optional in this case)
    // puback += (uint8_t)reason_code::SUCCESS;
    // // No properties to send
    // puback += uint8_t{0};

    m_messages.push(puback_ptr);
}

void mqtt::client::enqueue_pubrec(uint16_t packet_id) {
    packet *pubrec_ptr = new packet(mqtt::packet_type::PUBREC);
    packet& pubrec = *pubrec_ptr;
    // Set packet id
    pubrec += packet_id;
    // For now assume all packets are successful (reason code and properties are optional in this case)
    // pubrec += (uint8_t)reason_code::SUCCESS;
    // // No properties to send
    // pubrec += uint8_t{0};

    m_messages.push(pubrec_ptr);
}

void mqtt::client::enqueue_pubrel(uint16_t packet_id) {
    // Bit 1 of pubrel type must be set
    packet *pubrel_ptr = new packet((mqtt::packet_type)((uint8_t)mqtt::packet_type::PUBREL | (1 << 1)));
    packet& pubrel = *pubrel_ptr;
    // Set packet id
    pubrel += packet_id;
    // For now assume all packets are successful (reason code and properties are optional in this case)
    // pubrel += (uint8_t)reason_code::SUCCESS;
    // // No properties to send
    // pubrel += uint8_t{0};

    m_messages.push(pubrel_ptr);
}

void mqtt::client::enqueue_pubcomp(uint16_t packet_id) {
    packet *pubcomp_ptr = new packet(mqtt::packet_type::PUBCOMP);
    packet& pubcomp = *pubcomp_ptr;
    // Set packet id
    pubcomp += packet_id;
    // For now assume all packets are successful (reason code and properties are optional in this case)
    // pubcomp += (uint8_t)reason_code::SUCCESS;
    // // No properties to send
    // pubcomp += uint8_t{0};

    m_messages.push(pubcomp_ptr);
}

void mqtt::client::run() {
    while(true) {
        if(!m_tcp->connected() && m_state != mqtt::client::state::connecting) {
            m_state = mqtt::client::state::connecting;
            m_tcp->on_connected(std::bind(&mqtt::client::tcp_connected_callback, this));
            m_tcp->connect(m_host, m_port);
        } else if(m_tcp->connected() && m_messages.size() > 0) {
            packet* to_send = m_messages.front();
            m_messages.pop();
            m_tcp->write(to_send->serialize());
            int qos = to_send->qos();
            mqtt::packet_type masked = (mqtt::packet_type)((uint8_t)to_send->m_type & (uint8_t)mqtt::packet_type::MASK);
            if(
                (masked == mqtt::packet_type::PUBLISH && qos > 0) 
                || (masked != mqtt::packet_type::PUBCOMP && qos == 2)
            ) {
                m_unacked_sends.push(to_send);
            }
        } else if(m_tcp->connected() && m_unacked_recvs.size() > 0) {
            packet* to_ack = m_unacked_recvs.front();
            uint16_t packet_id;
            mqtt::packet_type masked = (mqtt::packet_type)((uint8_t)to_ack->m_type & (uint8_t)mqtt::packet_type::MASK);
            switch(masked) {
            case mqtt::packet_type::PUBLISH:{
                std::span<uint8_t> contents = to_ack->contents();
                std::span<uint8_t> topic = parse_binary(contents);
                packet_id = mqtt::get_u16(contents.subspan(topic.size()));
                break;
            }
            case mqtt::packet_type::PUBREC:
            case mqtt::packet_type::PUBREL:
                packet_id = mqtt::get_u16(to_ack->contents());
                break;
            }
            switch(masked) {
            case mqtt::packet_type::PUBLISH:
                if(to_ack->qos() == 1) {
                    enqueue_puback(packet_id);
                } else if(to_ack->qos() == 2) {
                    enqueue_pubrec(packet_id);
                }
                break;
            case mqtt::packet_type::PUBREC:
                enqueue_pubrel(packet_id);
                break;
            case mqtt::packet_type::PUBREL:
                enqueue_pubcomp(packet_id);
                break;
            }
        } else {
            sleep_ms(100);
        }
    }
}