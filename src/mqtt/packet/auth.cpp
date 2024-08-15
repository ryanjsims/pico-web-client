#include <mqtt/packet/auth.h>

mqtt::auth_packet::auth_packet(reason_code reason, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::AUTH);

    packet& auth = *m_packet;

    auth += (uint8_t)reason;
    auth += m_properties;
}

mqtt::auth_packet::auth_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
    , m_properties(packet->contents().subspan(props_offset()))
{}

mqtt::auth_packet::~auth_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

size_t mqtt::auth_packet::reason_offset() const {
    return 0;
}

size_t mqtt::auth_packet::props_offset() const {
    return 1;
}

mqtt::packet_type mqtt::auth_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::auth_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

mqtt::reason_code mqtt::auth_packet::reason() const {
    if(!m_packet) {
        return mqtt::reason_code::ERROR_UNSPECIFIED;
    }
    if(m_packet->size() == 0) {
        return mqtt::reason_code::SUCCESS;
    }
    return (mqtt::reason_code)m_packet->contents()[reason_offset()];
}

const mqtt::properties& mqtt::auth_packet::properties() const {
    return m_properties;
}

std::span<uint8_t> mqtt::auth_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::auth_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}