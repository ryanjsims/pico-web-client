#include <mqtt/packet/pubcomp.h>

mqtt::pubcomp_packet::pubcomp_packet(uint16_t pkt_id, reason_code reason, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::PUBCOMP);

    packet& pubcomp = *m_packet;

    pubcomp += pkt_id;
    pubcomp += (uint8_t)reason;
    pubcomp += m_properties;
}

mqtt::pubcomp_packet::pubcomp_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
    , m_properties(packet->contents().subspan(props_offset()))
{}

mqtt::pubcomp_packet::~pubcomp_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

size_t mqtt::pubcomp_packet::id_offset() const {
    return 0;
}

size_t mqtt::pubcomp_packet::reason_offset() const {
    return 2;
}

size_t mqtt::pubcomp_packet::props_offset() const {
    return 3;
}

mqtt::packet_type mqtt::pubcomp_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::pubcomp_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

uint16_t mqtt::pubcomp_packet::id() const {
    std::span<uint8_t> data = m_packet->contents();
    return (data[id_offset()] << 8) | data[id_offset()];
}

mqtt::reason_code mqtt::pubcomp_packet::reason() const {
    if(!m_packet) {
        return mqtt::reason_code::ERROR_UNSPECIFIED;
    }
    if(m_packet->size() == 2) {
        return mqtt::reason_code::SUCCESS;
    }
    return (mqtt::reason_code)m_packet->contents()[reason_offset()];
}

const mqtt::properties& mqtt::pubcomp_packet::properties() const {
    return m_properties;
}

std::span<uint8_t> mqtt::pubcomp_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::pubcomp_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}