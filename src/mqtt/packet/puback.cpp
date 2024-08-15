#include <mqtt/packet/puback.h>

mqtt::puback_packet::puback_packet(uint16_t pkt_id, reason_code reason, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::PUBACK);

    packet& puback = *m_packet;

    puback += pkt_id;
    puback += (uint8_t)reason;
    puback += m_properties;
}

mqtt::puback_packet::puback_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
    , m_properties(packet->contents().subspan(props_offset()))
{}

mqtt::puback_packet::~puback_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

size_t mqtt::puback_packet::id_offset() const {
    return 0;
}

size_t mqtt::puback_packet::reason_offset() const {
    return 2;
}

size_t mqtt::puback_packet::props_offset() const {
    return 3;
}

mqtt::packet_type mqtt::puback_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::puback_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

uint16_t mqtt::puback_packet::id() const {
    std::span<uint8_t> data = m_packet->contents();
    return (data[id_offset()] << 8) | data[id_offset()];
}

mqtt::reason_code mqtt::puback_packet::reason() const {
    if(!m_packet) {
        return mqtt::reason_code::ERROR_UNSPECIFIED;
    }
    if(m_packet->size() == 2) {
        return mqtt::reason_code::SUCCESS;
    }
    return (mqtt::reason_code)m_packet->contents()[reason_offset()];
}

const mqtt::properties& mqtt::puback_packet::properties() const {
    return m_properties;
}

std::span<uint8_t> mqtt::puback_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::puback_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}