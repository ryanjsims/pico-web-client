#include <mqtt/packet/pubrel.h>

mqtt::pubrel_packet::pubrel_packet(uint16_t pkt_id, reason_code reason, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::PUBREL);

    packet& pubrel = *m_packet;

    pubrel += pkt_id;
    pubrel += (uint8_t)reason;
    pubrel += m_properties;
}

mqtt::pubrel_packet::pubrel_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
    , m_properties(packet->contents().subspan(props_offset()))
{}

mqtt::pubrel_packet::~pubrel_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

size_t mqtt::pubrel_packet::id_offset() const {
    return 0;
}

size_t mqtt::pubrel_packet::reason_offset() const {
    return 2;
}

size_t mqtt::pubrel_packet::props_offset() const {
    return 3;
}

mqtt::packet_type mqtt::pubrel_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::pubrel_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

uint16_t mqtt::pubrel_packet::id() const {
    std::span<uint8_t> data = m_packet->contents();
    return (data[id_offset()] << 8) | data[id_offset()];
}

mqtt::reason_code mqtt::pubrel_packet::reason() const {
    if(!m_packet) {
        return mqtt::reason_code::ERROR_UNSPECIFIED;
    }
    if(m_packet->size() == 2) {
        return mqtt::reason_code::SUCCESS;
    }
    return (mqtt::reason_code)m_packet->contents()[reason_offset()];
}

const mqtt::properties& mqtt::pubrel_packet::properties() const {
    return m_properties;
}

std::span<uint8_t> mqtt::pubrel_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::pubrel_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}