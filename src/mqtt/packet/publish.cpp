#include <mqtt/packet/publish.h>

mqtt::publish_packet::publish_packet(flags_t flags, std::u8string topic_name, uint16_t pkt_id, std::span<uint8_t> payload, mqtt::properties properties)
    : m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet((mqtt::packet_type)((uint8_t)mqtt::packet_type::PUBLISH | (flags & 0x0F)));

    packet& publish = *m_packet;

    publish += topic_name;
    if(flags.qos()) {
        publish += pkt_id;
    }
    publish += m_properties;
    publish.add_raw(payload);
}

mqtt::publish_packet::publish_packet(mqtt::packet* packet) 
    : m_owned(false)
    , m_packet(packet)
    , m_properties(packet->contents().subspan(props_offset()))
{}

mqtt::publish_packet::~publish_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }
}

size_t mqtt::publish_packet::topic_offset() const {
    return 0;
}

size_t mqtt::publish_packet::topic_length() const {
    std::span<uint8_t> data = m_packet->contents();
    return (data[topic_offset()] << 8) | data[topic_offset() + 1];
}

size_t mqtt::publish_packet::id_offset() const {
    return 2 + topic_length();
}

size_t mqtt::publish_packet::props_offset() const {
    size_t offset = id_offset();
    if(flags().qos() > 0) {
        return offset + 2;
    }
    return offset;
}

size_t mqtt::publish_packet::payload_offset() const {
    const mqtt::properties& props = properties();
    return props_offset() + props.m_length + props.m_length.length;
}

mqtt::packet_type mqtt::publish_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->masked();
}

mqtt::varint_t mqtt::publish_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

mqtt::publish_packet::flags_t mqtt::publish_packet::flags() const {
    if(!m_packet) {
        return {0};
    }
    return {(uint8_t)m_packet->m_type};
}

std::u8string_view mqtt::publish_packet::topic() const {
    if(!m_packet) {
        return {};
    }
    std::span<uint8_t> data = m_packet->contents();
    return {(char8_t*)data.subspan(topic_offset() + 2).data(), topic_length()};
}

std::optional<uint16_t> mqtt::publish_packet::id() const {
    if(flags().qos() == 0) {
        return {};
    }
    std::span<uint8_t> data = m_packet->contents();
    return (data[id_offset()] << 8) | data[id_offset() + 1];
}

const mqtt::properties& mqtt::publish_packet::properties() const {
    return m_properties;
}

std::span<uint8_t> mqtt::publish_packet::payload() const {
    return m_packet->contents().subspan(payload_offset());
}

std::span<uint8_t> mqtt::publish_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::publish_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    m_owned = false;
    return to_return;
}