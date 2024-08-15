#include <mqtt/packet/connack.h>

mqtt::connack_packet::connack_packet(flags_t flags, reason_code reason, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::CONNACK);

    packet& connack = *m_packet;

    // flags
    connack += flags.value;
    // reason code
    connack += (uint8_t)reason;
    // properties
    connack += m_properties;
}

mqtt::connack_packet::connack_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
    , m_properties(packet->contents().subspan(props_offset()))
{}

mqtt::connack_packet::~connack_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

size_t mqtt::connack_packet::flags_offset() const {
    return 0;
}

size_t mqtt::connack_packet::reason_offset() const {
    return 1;
}

size_t mqtt::connack_packet::props_offset() const {
    return 2;
}

mqtt::packet_type mqtt::connack_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::connack_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

mqtt::connack_packet::flags_t mqtt::connack_packet::flags() const {
    if(!m_packet) {
        return {0};
    }
    return {m_packet->contents()[flags_offset()]};
}

mqtt::reason_code mqtt::connack_packet::reason() const {
    if(!m_packet) {
        return mqtt::reason_code::ERROR_UNSPECIFIED;
    }
    return (mqtt::reason_code)m_packet->contents()[reason_offset()];
}

const mqtt::properties& mqtt::connack_packet::properties() const {
    return m_properties;
}

std::span<uint8_t> mqtt::connack_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::connack_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}