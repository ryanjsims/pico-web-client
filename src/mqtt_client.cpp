#include <mqtt_client.h>
#include <mqtt/packets.h>

#include <logger.h>
#include <tcp_client.h>
#include <tcp_tls_client.h>

#include <cstring>
#include "LUrlParser.h"

namespace mqtt {
    uint16_t get_u16(std::span<uint8_t> data) {
        return (uint16_t)((data[0] << 8) | data[1]);
    }
}

mqtt::client::client(
    std::string url,
    std::span<uint8_t> cert
)
    : m_url(url)
    , m_host("")
    , m_port(-1)
    , m_cert(cert)
    , m_tcp(nullptr)
    , m_state(mqtt::client::state::disconnected)
    , m_client_id(u8"")
    , m_current_packet_id(1)
{
    parse_url();
    m_tcp->on_receive(std::bind(&mqtt::client::tcp_recv_callback, this));
    m_tcp->on_send(std::bind(&mqtt::client::tcp_send_callback, this, std::placeholders::_1));
    m_tcp->on_closed(std::bind(&mqtt::client::tcp_closed_callback, this));
    m_tcp->on_error(std::bind(&mqtt::client::tcp_error_callback, this, std::placeholders::_1));
}

mqtt::client::~client() {
    if(m_tcp) {
        delete m_tcp;
    }
}

void mqtt::client::connect() {
    connect(u8"", std::span<uint8_t>{});
}

void mqtt::client::connect(std::u8string username, std::u8string password) {
    connect(username, std::span<uint8_t>{(uint8_t*)password.data(), password.size()});
}

void mqtt::client::connect(std::u8string username, std::span<uint8_t> password) {
    mqtt::connect_packet packet{m_client_id, username, password};
    m_send_queue.push(packet.release());

    m_state = mqtt::client::state::connecting;
    m_tcp->on_connected(std::bind(&mqtt::client::tcp_connected_callback, this));
    m_tcp->connect(m_host, m_port);
}

void mqtt::client::disconnect(reason_code reason) {
    mqtt::disconnect_packet packet(reason);
    m_send_queue.push(packet.release());
    m_state = state::disconnecting;
}

void mqtt::client::subscribe(std::u8string topic_filter, subscribe_packet::options_t options, publish_handler_t handler) {
    subscribe({&topic_filter, 1}, {&options, 1}, handler);
}

void mqtt::client::subscribe(std::span<std::u8string> topic_filters, std::span<subscribe_packet::options_t> options, publish_handler_t handler) {
    subscription_t sub = {
        {topic_filters.begin(), topic_filters.end()},
        {},
        handler,
        true // We could start receiving events right away
    };

    info("mqtt::client::subscribe: have %d stored subs\n", m_subscriptions.size());

    uint i;
    for(i = 0; i < m_subscriptions.size(); i++) {
        if(!m_subscriptions[i].active) {
            m_subscriptions[i] = sub;
            break;
        }
    }
    if(i == m_subscriptions.size()) {
        m_subscriptions.push_back(sub);
    }
    info("mqtt::client::subscribe: adding subscription with id %d\n", i);

    mqtt::properties properties;
    varint_t value(i);
    uint8_t data[value.length];
    value.serialize({data, value.length});
    property subscription_id{property_name::SUB_ID, {data, value.length}};
    properties.push_back(subscription_id);

    subscribe_packet packet(generate_packet_id(), topic_filters, options, properties);
    m_send_queue.push(packet.release());
}

void mqtt::client::unsubscribe(std::u8string topic_filter) {
    unsubscribe({&topic_filter, 1});
}

void mqtt::client::unsubscribe(std::span<std::u8string> topic_filters) {
    unsubscribe_packet packet(generate_packet_id(), topic_filters);

    m_send_queue.push(packet.release());
}

void mqtt::client::publish(std::u8string topic, publish_packet::flags_t flags, std::span<uint8_t> data) {
    publish(topic, flags, u8"", data);
}

void mqtt::client::publish(std::u8string topic, publish_packet::flags_t flags, std::u8string content_type, std::span<uint8_t> data) {
    mqtt::packet* to_send;
    if(content_type.size() > 0) {
        mqtt::properties properties;
        uint8_t ct_data[content_type.size() + 2];
        ct_data[0] = (content_type.size() >> 8) & 0xFF;
        ct_data[1] = content_type.size() & 0xFF;
        memcpy(&ct_data[2], content_type.data(), content_type.size());
        property content_type_property(property_name::CONTENT_TYPE, {ct_data, content_type.size() + 2});
        properties.push_back(content_type_property);
        publish_packet packet(flags, topic, generate_packet_id(), data, properties);
        to_send = packet.release();
    } else {
        publish_packet packet(flags, topic, generate_packet_id(), data);\
        to_send = packet.release();
    }
    m_send_queue.push(to_send);
}

bool mqtt::client::connected() {
    return m_state == state::connected;
}

uint16_t mqtt::client::generate_packet_id() {
    uint16_t to_return = m_current_packet_id;
    if(m_current_packet_id == 0xFFFE) {
        m_current_packet_id = 1;
    } else {
        m_current_packet_id++;
    }
    return to_return;
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

bool mqtt::client::queue_timer_callback(repeating_timer_t* rt) {
    mqtt::client* client = (mqtt::client*)rt->user_data;
    client->handle_packet_queues();
    return true;
}

mqtt::packet* mqtt::client::get_next_packet() {
    packet* to_send = m_send_queue.front();
    m_send_queue.pop();

    // if this is a QoS 1/2 publish and we're at our quota, look for another to send
    uint i;
    for(
        i = 0; 
        i < m_send_queue.size() &&
        to_send->masked() == packet_type::PUBLISH &&
        to_send->qos() > 0 &&
        m_send_quota == 0; 
        i++
    ) {
        m_send_queue.push(to_send);

        to_send = m_send_queue.front();
        m_send_queue.pop();
    }
    // if we've looked at the entire queue and none can be sent, abort this send
    if(i == m_send_queue.size() && to_send->masked() == packet_type::PUBLISH && to_send->qos() > 0) {
        debug1("mqtt::client::handle_packet_queues: at QoS quota, no messages to send!\n");
        m_send_queue.push(to_send);
        return nullptr;
    }
    return to_send;
}

void mqtt::client::handle_packet_queues() {
    packet* to_send;
    if(m_tcp->connected() && m_send_queue.size() > 0 && (to_send = get_next_packet())) {
        std::span<uint8_t> data = to_send->serialize();
        dump_bytes_debug(data.data(), data.size());
        m_tcp->write(data);

        mqtt::packet_type masked = to_send->masked();
        switch(masked) {
        case mqtt::packet_type::PUBLISH:
            if(to_send->qos() == 0) {
                delete to_send;
                break;
            }
        case mqtt::packet_type::PUBREC:
        case mqtt::packet_type::PUBREL:
        case mqtt::packet_type::SUBSCRIBE:
        case mqtt::packet_type::UNSUBSCRIBE:
        case mqtt::packet_type::PINGREQ:
            m_unacked_sends.push(to_send);
            break;
        case mqtt::packet_type::DISCONNECT:
            m_state = state::disconnected;
        default:
            delete to_send;
        }
        std::string packet_name = packet_type_string(masked);
        info("Sending %.*s packet\n", packet_name.size(), packet_name.data());
    } 
    if(m_tcp->connected() && m_recv_queue.size() > 0) {
        info1("Handling recv packet...\n");
        packet* recved = m_recv_queue.front();
        m_recv_queue.pop();
        mqtt::packet_type masked = recved->masked();
        switch(masked) {
        case mqtt::packet_type::CONNECT:
            handle_connect(recved);
            break;
        case mqtt::packet_type::CONNACK:
            handle_connack(recved);
            break;
        case mqtt::packet_type::PUBLISH:
            handle_publish(recved);
            break;
        case mqtt::packet_type::PUBACK:
            handle_puback(recved);
            break;
        case mqtt::packet_type::PUBREC:
            handle_pubrec(recved);
            break;
        case mqtt::packet_type::PUBREL:
            handle_pubrel(recved);
            break;
        case mqtt::packet_type::PUBCOMP:
            handle_pubcomp(recved);
            break;
        case mqtt::packet_type::SUBSCRIBE:
            handle_subscribe(recved);
            break;
        case mqtt::packet_type::SUBACK:
            handle_suback(recved);
            break;
        case mqtt::packet_type::UNSUBSCRIBE:
            handle_unsubscribe(recved);
            break;
        case mqtt::packet_type::UNSUBACK:
            handle_unsuback(recved);
            break;
        case mqtt::packet_type::PINGREQ:
            handle_pingreq(recved);
            break;
        case mqtt::packet_type::PINGRESP:
            handle_pingresp(recved);
            break;
        case mqtt::packet_type::DISCONNECT:
            handle_disconnect(recved);
            break;
        case mqtt::packet_type::AUTH:
            handle_auth(recved);
            break;
        }
    }
}

mqtt::packet* mqtt::client::get_unacked(packet_type type, uint16_t packet_id) {
    mqtt::packet* to_return = nullptr;
    for(uint i = 0; i < m_unacked_sends.size(); i++) {
        mqtt::packet* unacked = m_unacked_sends.front();
        m_unacked_sends.pop();
        if(unacked->masked() == type) {
            uint16_t unacked_id;
            switch(type) {
            case packet_type::PUBLISH:{
                publish_packet pub(unacked);
                unacked_id = *pub.id();
                break;
            }
            case packet_type::PUBREC:{
                pubrec_packet rec(unacked);
                unacked_id = rec.id();
                break;
            }
            case packet_type::PUBREL:{
                pubrel_packet rel(unacked);
                unacked_id = rel.id();
                break;
            }
            case packet_type::SUBSCRIBE:{
                subscribe_packet sub(unacked);
                unacked_id = sub.id();
                break;
            }
            case packet_type::UNSUBSCRIBE:{
                unsubscribe_packet unsub(unacked);
                unacked_id = unsub.id();
                break;
            }
            case packet_type::PINGREQ:
                unacked_id = 0xFFFF;
                break;
            default:
                warn("mqtt::client::get_unacked: Unhandled type %02x\n", (uint8_t)type);
                unacked_id = 0xFFFF;
                break;
            }
            if(packet_id == unacked_id && to_return == nullptr) {
                to_return = unacked;
            } else {
                m_unacked_sends.push(unacked);
            }
        } else {
            m_unacked_sends.push(unacked);
        }
    }
    return to_return;
}

void mqtt::client::clear_unacked() {
    while(m_unacked_sends.size() > 0) {
        delete m_unacked_sends.front();
        m_unacked_sends.pop();
    }
}

void mqtt::client::resend_reconnect() {
    for(uint i = 0; i < m_unacked_sends.size(); i++) {
        mqtt::packet* unacked = m_unacked_sends.front();
        m_unacked_sends.pop();
        switch(unacked->masked()) {
        case packet_type::PUBLISH:
        case packet_type::PUBREL:
            m_send_queue.push(unacked);
            break;
        default:
            m_unacked_sends.push(unacked);
            break;
        }
    }
}

void mqtt::client::handle_connect(mqtt::packet* packet) {
    debug1("mqtt::client::handle_connect\n");
    // Clients shouldn't receive connects...
    disconnect(reason_code::ERROR_PROTOCOL);
    delete packet;
}

void mqtt::client::handle_connack(mqtt::packet* packet) {
    debug1("mqtt::client::handle_connack\n");
    connack_packet ack(packet);

    bool session_present = ack.flags().session_present();
    if(session_present && m_client_id.size() == 0) {
        // No previous session with this client
        disconnect(reason_code::ERROR_UNSPECIFIED);
    } else if(session_present && m_client_id.size() > 0) {
        resend_reconnect();
    } else {
        clear_unacked();
        m_subscriptions.clear();
        // needed so sub ids are in the range 1-268,435,455
        m_subscriptions.push_back({{}, {}, NULL_SUB_HANDLER, true});
    }

    std::vector<const property*> recv_max = ack.properties()[property_name::RECV_MAX];
    if(recv_max.size() > 1) {
        disconnect(reason_code::ERROR_PROTOCOL);
    } else if(recv_max.size() == 1) {
        m_send_quota = recv_max[0]->as_twobyte();
    } else {
        m_send_quota = 0xFFFF;
    }
    if(m_send_quota > 64) {
        m_send_quota = 64;
    } else if(m_send_quota == 0) {
        disconnect(reason_code::ERROR_PROTOCOL);
    }

    std::vector<const property*> max_qos = ack.properties()[property_name::MAX_QOS];
    if(max_qos.size() > 1) {
        disconnect(reason_code::ERROR_PROTOCOL);
    } else if(max_qos.size() == 1) {
        m_max_qos = max_qos[0]->as_byte();
    } else {
        m_max_qos = 2;
    }

    std::vector<const property*> client_id = ack.properties()[property_name::CLIENT_ID];
    if(client_id.size() == 1) {
        debug1("Setting client ID from properties...\n");
        dump_bytes_debug((uint8_t*)client_id[0]->as_string().data(), client_id[0]->as_string().size());
        m_client_id = client_id[0]->as_string();
    }

    if(ack.reason() == reason_code::SUCCESS && m_state == state::connecting) {
        m_state = state::connected;
        info("MQTT connected with client id '%.*s'\n\tMax QoS:  %d\n\tRecv Max: %d\n", m_client_id.size(), m_client_id.data(), m_max_qos, m_send_quota);
    } else {
        m_state = state::disconnecting;
    }
    delete packet;
}

void mqtt::client::handle_publish(mqtt::packet* packet) {
    debug1("mqtt::client::handle_publish\n");
    publish_packet pub(packet);
    uint16_t packet_id = *pub.id();
    reason_code reason = mqtt::reason_code::SUCCESS;
    std::vector<const property*> sub_ids;
    mqtt::packet* unacked;
    if(pub.flags().qos() == 2 && (unacked = get_unacked(packet_type::PUBREC, packet_id)) != nullptr) {
        // Clear unacked pubrec packet
        // Leave sub_ids empty so duplicate packet is not sent to handlers
        // [MQTT-4.3.3-10]
        delete unacked;
    } else {
        // This publish's id has not been seen by us yet, was rec'ed with an error code, or has a qos of < 2
        // Find sub_ids to deliver publish
        sub_ids = pub.properties()[property_name::SUB_ID];
    }

    // call subscription handler for all sub ids
    // If any fail, use the returned reason code in the ack
    // If no subscription could be found, this is an unsolicited publish
    for(uint i = 0; i < sub_ids.size(); i++) {
        varint_t sub_id = sub_ids[i]->as_varint();
        if(sub_id < m_subscriptions.size() && m_subscriptions[sub_id].active) {
            reason_code temp = m_subscriptions[sub_id].handler(pub.topic(), pub.properties(), pub.payload());
            if(reason == reason_code::SUCCESS && temp != reason_code::SUCCESS) {
                reason = temp;
            }
        }
    }

    if(packet->qos() == 1) {
        puback_packet ack(packet_id, reason);
        m_send_queue.push(ack.release());
    } else if(packet->qos() == 2) {
        pubrec_packet rec(packet_id, reason);
        mqtt::packet *to_send = rec.release();
        m_send_queue.push(to_send);
    }
    delete packet;
}

void mqtt::client::handle_puback(mqtt::packet* packet) {
    debug1("mqtt::client::handle_puback\n");
    puback_packet ack(packet);
    uint16_t packet_id = ack.id();
    mqtt::packet* unacked = get_unacked(mqtt::packet_type::PUBLISH, packet_id);
    if(unacked) {
        delete unacked;
    }
    delete packet;
}

void mqtt::client::handle_pubrec(mqtt::packet* packet) {
    debug1("mqtt::client::handle_pubrec\n");
    pubrec_packet rec(packet);
    uint16_t packet_id = rec.id();
    mqtt::reason_code reason = reason_code::ERROR_PKT_ID_DNE;
    mqtt::packet* unacked = get_unacked(mqtt::packet_type::PUBLISH, packet_id);
    if(unacked) {
        reason = reason_code::SUCCESS;
        delete unacked;
    }

    pubrel_packet rel(packet_id, reason);
    mqtt::packet* to_send = rel.release();
    m_send_queue.push(to_send);
    delete packet;
}

void mqtt::client::handle_pubrel(mqtt::packet* packet) {
    debug1("mqtt::client::handle_pubrel\n");
    pubrel_packet rel(packet);
    uint16_t packet_id = rel.id();
    mqtt::reason_code reason = reason_code::ERROR_PKT_ID_DNE;
    mqtt::packet* unacked = get_unacked(mqtt::packet_type::PUBREC, packet_id);
    if(unacked) {
        reason = reason_code::SUCCESS;
        delete unacked;
    }

    pubcomp_packet comp(packet_id, reason);
    m_send_queue.push(comp.release());
    delete packet;
}

void mqtt::client::handle_pubcomp(mqtt::packet* packet) {
    debug1("mqtt::client::handle_pubcomp\n");
    pubcomp_packet comp(packet);
    uint16_t packet_id = comp.id();
    mqtt::packet* unacked = get_unacked(mqtt::packet_type::PUBREL, packet_id);
    if(unacked) {
        delete unacked;
    }
    delete packet;
}

void mqtt::client::handle_subscribe(mqtt::packet* packet) {
    debug1("mqtt::client::handle_subscribe\n");
    // Clients shouldn't receive subscribes...
    disconnect(reason_code::ERROR_PROTOCOL);
    delete packet;
}

void mqtt::client::handle_suback(mqtt::packet* packet) {
    debug1("mqtt::client::handle_suback\n");
    suback_packet ack(packet);
    uint16_t packet_id = ack.id();
    mqtt::packet* unacked = get_unacked(mqtt::packet_type::SUBSCRIBE, packet_id);

    if(!unacked) {
        delete packet;
        return;
    }

    subscribe_packet sub(unacked);
    varint_t sub_id = sub.properties()[property_name::SUB_ID][0]->as_varint();

    std::span<reason_code> reasons = ack.reasons();
    std::u8string_view topic;
    for(int i = 0; i < reasons.size(); i++) {
        switch(reasons[i]) {
        case reason_code::GRANTED_QoS_0:
        case reason_code::GRANTED_QoS_1:
        case reason_code::GRANTED_QoS_2:
            m_subscriptions[sub_id].max_qos.push_back((uint8_t)reasons[i]);
            break;
        default:
            topic = sub.topic_filter(i);
            error("Error 0x%02x subscribing to topic filter %.*s\n", reasons[i], topic.size(), topic.data());
            m_subscriptions[sub_id].topic_filters[i] = u8"";
            m_subscriptions[sub_id].max_qos.push_back((uint8_t)reasons[i]);
            break;
        }
    }

    if(unacked) {
        delete unacked;
    }
    delete packet;
}

void mqtt::client::handle_unsubscribe(mqtt::packet* packet) {
    debug1("mqtt::client::handle_unsubscribe\n");
    // Clients shouldn't receive unsubscribes...
    disconnect(reason_code::ERROR_PROTOCOL);
    delete packet;
}

void mqtt::client::handle_unsuback(mqtt::packet* packet) {
    debug1("mqtt::client::handle_unsuback\n");
    unsuback_packet ack(packet);
    uint16_t packet_id = ack.id();
    mqtt::packet* unacked = get_unacked(mqtt::packet_type::UNSUBSCRIBE, packet_id);

    if(!unacked) {
        delete packet;
        return;
    }

    unsubscribe_packet unsub(unacked);
    std::span<reason_code> reasons = ack.reasons();
    std::u8string_view topic_filter;
    for(uint i = 0; i < reasons.size(); i++) {
        if(reasons[i] == reason_code::ERROR_PKT_ID_USED) {
            error1("Unsubscribe packet id was in use!\n");
            break;
        }
        topic_filter = unsub.topic_filter(i);
        switch(reasons[i]) {
        case reason_code::SUCCESS:
            for(uint j = 0; j < m_subscriptions.size(); j++) {
                if(!m_subscriptions[j].active) {
                    continue;
                }
                for(uint k = 0; k < m_subscriptions[j].topic_filters.size(); k++) {
                    if(topic_filter == m_subscriptions[j].topic_filters[k]) {
                        m_subscriptions[j].active = false;
                        m_subscriptions[j].topic_filters.clear();
                        m_subscriptions[j].max_qos.clear();
                        break;
                    }
                }
            }
            break;
        case reason_code::NO_EXISTING_SUB:
            warn("Client could not unsubscribe from '%.*s' - no subscription was registered!\n", topic_filter.size(), topic_filter.data());
            break;
        default:
            error("mqtt::client::handle_unsuback: code 0x%02x for topic filter '%.*s'\n", topic_filter.size(), topic_filter.data());
            break;
        }
    }

    delete unacked;
    delete packet;
}

void mqtt::client::handle_pingreq(mqtt::packet* packet) {
    debug1("mqtt::client::handle_pingreq\n");
    // Clients shouldn't receive ping requests...
    disconnect(reason_code::ERROR_PROTOCOL);
    delete packet;
}

void mqtt::client::handle_pingresp(mqtt::packet* packet) {
    debug1("mqtt::client::handle_pingresp\n");
    pingresp_packet resp(packet);
    mqtt::packet* unacked = get_unacked(mqtt::packet_type::PINGREQ, 0xFFFF);

    delete unacked;
    delete packet;
}

void mqtt::client::handle_disconnect(mqtt::packet* packet) {
    debug1("mqtt::client::handle_disconnect\n");
    disconnect_packet disc(packet);

    info("Disconnected with reason 0x%02x\n", disc.reason());
    m_state = state::disconnected;

    delete packet;
}

void mqtt::client::handle_auth(mqtt::packet* packet) {
    debug1("mqtt::client::handle_auth\n");
    error1("Extended authentication not currently supported\n");
    disconnect(reason_code::ERROR_IMPL_SPECIFIC);
    delete packet;
}

void mqtt::client::tcp_connected_callback() {
    debug1("mqtt::client::tcp_connected_callback\n");
    add_repeating_timer_ms(50, mqtt::client::queue_timer_callback, this, &queue_timer);
}

void mqtt::client::tcp_recv_callback() {
    debug1("mqtt::client::tcp_recv_callback\n");
    uint8_t data[m_tcp->available()];
    std::span<uint8_t> span = {(uint8_t*)data, (size_t)m_tcp->available()};
    m_tcp->read(span);
    #if LOG_LEVEL <= LOG_LEVEL_DEBUG
    debug("mqtt::client recv'd %d bytes\n", span.size());
    dump_bytes_debug(span.data(), span.size());
    #endif
    mqtt::packet* received = new mqtt::packet(span);
    if(received == nullptr) {
        error1("Failed to allocate packet memory\n");
        return;
    }
    m_recv_queue.push(received);
    std::string packet_name = packet_type_string(received->masked());
    info("Received %.*s packet\n", packet_name.size(), packet_name.data());
}

void mqtt::client::tcp_send_callback(uint16_t len) {
    if(m_state == state::disconnecting) {
        m_state = state::disconnected;
        m_tcp->close(ERR_CLSD);
    }
}

void mqtt::client::tcp_closed_callback() {
    debug1("mqtt::client::tcp_closed_callback\n");
    cancel_repeating_timer(&queue_timer);
}

void mqtt::client::tcp_error_callback(err_t err) {
    debug1("mqtt::client::tcp_error_callback\n");
    error("Got error: '%s'\n", tcp_perror(err).c_str());
    cancel_repeating_timer(&queue_timer);
}
