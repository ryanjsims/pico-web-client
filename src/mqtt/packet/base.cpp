#include <mqtt/packet/base.h>

#include "logger.h"
#include <cstring>

std::string mqtt::packet_type_string(mqtt::packet_type type) {
    switch(type) {
        case packet_type::UNDEFINED:
            return "undefined";
        case packet_type::CONNECT:
            return "connect";
        case packet_type::CONNACK:
            return "connack";
        case packet_type::PUBLISH:
            return "publish";
        case packet_type::PUBACK:
            return "puback";
        case packet_type::PUBREC:
            return "pubrec";
        case packet_type::PUBREL:
            return "pubrel";
        case packet_type::PUBCOMP:
            return "pubcomp";
        case packet_type::SUBSCRIBE:
            return "subscribe";
        case packet_type::SUBACK:
            return "suback";
        case packet_type::UNSUBSCRIBE:
            return "unsubscribe";
        case packet_type::UNSUBACK:
            return "unsuback";
        case packet_type::PINGREQ:
            return "pingreq";
        case packet_type::PINGRESP:
            return "pingresp";
        case packet_type::DISCONNECT:
            return "disconnect";
        case packet_type::AUTH:
            return "auth";
        default:
            return "(unknown)";
    }
}

void mqtt::packet::expand_if_needed(uint32_t length_to_add) {
    if(data == nullptr) {
        m_capacity = 128;
        data = (uint8_t*)malloc(m_capacity);
    } else if((m_count + length_to_add) > m_capacity) {
        m_capacity *= 2;
        data = (uint8_t*)realloc(data, m_capacity);
    }
    m_data = {data + 5, m_capacity};
}

mqtt::packet& mqtt::packet::operator+=(const std::u8string& value) {
    expand_if_needed(value.size() + 2);
    m_data[m_count] = (uint8_t)(value.size() >> 8);
    m_data[m_count + 1] = (uint8_t)(value.size() & 0xFF);
    memcpy(m_data.data() + m_count + 2, value.data(), value.size());
    m_count += value.size() + 2;
    return (*this);
}

mqtt::packet& mqtt::packet::operator+=(const std::span<uint8_t>& value) {
    expand_if_needed(value.size() + 2);
    m_data[m_count] = (uint8_t)(value.size() >> 8);
    m_data[m_count + 1] = (uint8_t)(value.size() & 0xFF);
    memcpy(m_data.data() + m_count + 2, value.data(), value.size());
    m_count += value.size() + 2;
    return (*this);
}

mqtt::packet& mqtt::packet::operator+=(uint8_t value) {
    expand_if_needed(1);
    m_data[m_count] = value;
    m_count += 1;
    return (*this);
}

mqtt::packet& mqtt::packet::operator+=(uint16_t value) {
    expand_if_needed(2);
    m_data[m_count] = value >> 8;
    m_data[m_count + 1] = value & 0xFF;
    m_count += 2;
    return (*this);
}

mqtt::packet& mqtt::packet::operator+=(uint32_t value) {
    expand_if_needed(4);
    m_data[m_count] = value >> 24;
    m_data[m_count + 1] = (value >> 16) & 0xFF;
    m_data[m_count + 2] = (value >> 8) & 0xFF;
    m_data[m_count + 3] = value & 0xFF;
    m_count += 4;
    return (*this);
}

mqtt::packet& mqtt::packet::operator+=(varint_t value) {
    expand_if_needed(value.length);
    value.serialize(m_data.subspan(m_count, value.length));
    m_count += value.length;
    return (*this);
}

mqtt::packet& mqtt::packet::operator+=(const properties& value) {
    expand_if_needed(value.m_length + value.m_length.length);
    value.serialize(m_data.subspan(m_count, value.m_length + value.m_length.length));
    m_count += value.m_length + value.m_length.length;
    return (*this);
}

void mqtt::packet::add_raw(const std::span<uint8_t>& value) {
    expand_if_needed(value.size());
    memcpy(m_data.data() + m_count, value.data(), value.size());
    m_count += value.size();
}

mqtt::packet::packet(std::span<uint8_t> value) {
    m_type = (packet_type)value[0];
    m_length = varint_t{value.subspan(1)};
    data = (uint8_t*)malloc(m_length + 5);
    if(data == nullptr) {
        panic("mqtt::packet constructor: OOM when constructing packet\n");
    }
    memcpy(data + 5, value.data() + 1 + m_length.length, m_length);
    m_data = {data + 5, m_length};
    m_capacity = m_length;
    m_count = m_length;
}

std::span<uint8_t> mqtt::packet::serialize() {
    m_length = varint_t(m_count);
    m_length.serialize({data + 5 - m_length.length, m_length.length});
    data[4 - m_length.length] = (uint8_t)m_type;
    return {data + 4 - m_length.length, m_length + 1 + m_length.length};
}

mqtt::packet::~packet() {
    if(data) {
        free(data);
        data = nullptr;
        m_count = 0;
        m_data = {};
    }
}

const std::span<uint8_t> mqtt::packet::contents() {
    return m_data;
}

uint8_t mqtt::packet::qos() {
    mqtt::packet_type masked = (mqtt::packet_type)((uint8_t)m_type & (uint8_t)mqtt::packet_type::MASK);
    switch(masked) {
    case mqtt::packet_type::PUBLISH:
        return ((uint8_t)m_type & 0x06) >> 1;
    case mqtt::packet_type::PUBREC:
    case mqtt::packet_type::PUBREL:
    case mqtt::packet_type::PUBCOMP:
        return 2;
    case mqtt::packet_type::PUBACK:
        return 1;
    default:
        return 0;
    }
}
