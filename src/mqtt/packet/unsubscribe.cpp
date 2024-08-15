#include <mqtt/packet/unsubscribe.h>

#include <logger.h>

mqtt::unsubscribe_packet::unsubscribe_packet(uint16_t pkt_id, std::u8string topic_filter, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::UNSUBSCRIBE);

    packet& unsubscribe = *m_packet;

    // Variable Header
    unsubscribe += pkt_id;
    unsubscribe += m_properties;

    // Payload
    unsubscribe += topic_filter;
}

mqtt::unsubscribe_packet::unsubscribe_packet(uint16_t pkt_id, std::vector<std::u8string> topic_filters, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    if(topic_filters.size() == 0) {
        error1("mqtt::unsubscribe_packet constructor: Topic filters cannot be size 0!\n");
        panic("invalid unsubscribe packet");
    }

    m_owned = true;
    m_packet = new packet(mqtt::packet_type::SUBSCRIBE);

    packet& unsubscribe = *m_packet;

    // Variable Header
    unsubscribe += pkt_id;
    unsubscribe += m_properties;

    // Payload
    for(uint i = 0; i < topic_filters.size(); i++) {
        unsubscribe += topic_filters[i];
    }
}

mqtt::unsubscribe_packet::unsubscribe_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
    , m_properties(packet->contents().subspan(props_offset()))
{}

mqtt::unsubscribe_packet::~unsubscribe_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

size_t mqtt::unsubscribe_packet::id_offset() const {
    return 0;
}

size_t mqtt::unsubscribe_packet::props_offset() const {
    return 2;
}

size_t mqtt::unsubscribe_packet::payload_offset() const {
    const mqtt::properties& props = properties();
    return props_offset() + props.m_length + props.m_length.length;
}

mqtt::packet_type mqtt::unsubscribe_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::unsubscribe_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

uint16_t mqtt::unsubscribe_packet::id() const {
    std::span<uint8_t> data = m_packet->contents();
    return (data[id_offset()] << 8) | data[id_offset()];
}

const mqtt::properties& mqtt::unsubscribe_packet::properties() const {
    return m_properties;
}

std::u8string_view mqtt::unsubscribe_packet::topic_filter(uint index) const {
    size_t offset = payload_offset();
    std::span<uint8_t> data = m_packet->contents();
    uint16_t length = (data[offset] << 8) | data[offset + 1];
    while(index > 0 && offset < data.size()) {
        length = (data[offset] << 8) | data[offset + 1];
        index--;
        offset += length + 2;
    }
    if(index == 0 && offset < data.size()) {
        return {(char8_t*)data.subspan(offset + 2).data(), length};
    }
    return {};
}

std::span<uint8_t> mqtt::unsubscribe_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::unsubscribe_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}