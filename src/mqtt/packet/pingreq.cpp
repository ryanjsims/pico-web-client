#include <mqtt/packet/pingreq.h>

mqtt::pingreq_packet::pingreq_packet() {
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::PINGREQ);
}

mqtt::pingreq_packet::pingreq_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
{}

mqtt::pingreq_packet::~pingreq_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

mqtt::packet_type mqtt::pingreq_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::pingreq_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

std::span<uint8_t> mqtt::pingreq_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::pingreq_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}