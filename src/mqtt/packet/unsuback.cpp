#include <mqtt/packet/unsuback.h>

mqtt::unsuback_packet::unsuback_packet(uint16_t pkt_id, std::span<reason_code> reasons, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::UNSUBACK);

    packet& unsuback = *m_packet;

    unsuback += pkt_id;
    unsuback += m_properties;
    unsuback.add_raw({(uint8_t*)reasons.data(), reasons.size()});
}

mqtt::unsuback_packet::unsuback_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
    , m_properties(packet->contents().subspan(props_offset()))
{}

mqtt::unsuback_packet::~unsuback_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

size_t mqtt::unsuback_packet::id_offset() const {
    return 0;
}

size_t mqtt::unsuback_packet::props_offset() const {
    return 2;
}

size_t mqtt::unsuback_packet::reasons_offset() const {
    const mqtt::properties& props = properties();
    return props_offset() + props.m_length + props.m_length.length;
}

mqtt::packet_type mqtt::unsuback_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::unsuback_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

uint16_t mqtt::unsuback_packet::id() const {
    std::span<uint8_t> data = m_packet->contents();
    return (data[id_offset()] << 8) | data[id_offset() + 1];
}

std::span<mqtt::reason_code> mqtt::unsuback_packet::reasons() const {
    if(!m_packet) {
        return {};
    }
    std::span<uint8_t> data = m_packet->contents().subspan(reasons_offset());
    return {(reason_code*)data.data(), data.size()};
}

const mqtt::properties& mqtt::unsuback_packet::properties() const {
    return m_properties;
}

std::span<uint8_t> mqtt::unsuback_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::unsuback_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}