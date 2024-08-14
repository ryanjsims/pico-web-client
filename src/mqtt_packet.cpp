#include <mqtt_packet.h>

#include "logger.h"
#include <cstring>

mqtt::varint_t::varint_t(std::span<uint8_t> data) {
    value = 0;
    length = 0;
    uint8_t shift = 0;
    uint8_t encoded;
    do {
        encoded = data[0];
        value += (encoded & 0x7F) << shift;
        if(shift > 21) {
            value = -1;
            break;
        }
        shift += 7;
        length += 1;
        data = data.subspan(1);
    } while(encoded & 0x80);
}

mqtt::varint_t::varint_t(uint32_t curr_value) : value(curr_value) {
    uint8_t encoded;
    length = 0;
    do {
        curr_value = curr_value >> 7;
        if(length == 4) {
            return;
        }
        length++;
    } while(curr_value > 0);
}

uint8_t mqtt::varint_t::serialize(std::span<uint8_t> output) const {
    uint32_t curr_value = value;

    uint8_t encoded;
    uint8_t i = 0;
    do {
        encoded = curr_value & 0x7F;
        curr_value = curr_value >> 7;
        if(curr_value > 0) {
            encoded |= 0x80;
        }
        if(i == output.size()) {
            return i;
        }
        output[i] = encoded;
        i++;
    } while(curr_value > 0);
    return i;
}

mqtt::varint_t::operator int() const {
    return value;
}

std::span<uint8_t> mqtt::parse_binary(std::span<uint8_t> data) {
    uint16_t value_length = (data[0] << 8) | data[1];
    return data.first(value_length + 2);
}

std::span<uint8_t> mqtt::parse_string_pair(std::span<uint8_t> data) {
    uint16_t value1_length = (data[0] << 8) | data[1];
    uint16_t value2_length = (data[value1_length + 2] << 8) | data[value1_length + 3];
    return data.first(value1_length + value2_length + 4);
}

std::span<uint8_t> mqtt::parse_varint(std::span<uint8_t> data) {
    uint8_t i = 0;
    while(i < 4 && (data[i] & 0x80)) {
        i++;
    }
    return data.first(i);
}

mqtt::property::property(mqtt::property_name name, std::span<uint8_t> data) {
    switch(name) {
    case property_name::PAYLOAD_FMT:
    case property_name::REQ_PROB_INFO:
    case property_name::REQ_RESP_INFO:
    case property_name::MAX_QOS:
    case property_name::RETAIN_AVAIL:
    case property_name::WILD_SUB_AVAIL:
    case property_name::SUB_ID_AVAIL:
    case property_name::SHARED_SUB_AVAIL:
        m_data = data.first(1);
        break;
    case property_name::KEEP_ALIVE:
    case property_name::RECV_MAX:
    case property_name::TOPIC_ALIAS_MAX:
    case property_name::TOPIC_ALIAS:
        m_data = data.first(2);
        break;
    case property_name::MSG_EXPIRY:
    case property_name::SESS_EXPIRY:
    case property_name::WILL_DELAY:
    case property_name::MAX_PKT_SIZE:
        m_data = data.first(4);
        break;
    case property_name::CONTENT_TYPE:
    case property_name::RESP_TOPIC:
    case property_name::CORR_DATA:
    case property_name::CLIENT_ID:
    case property_name::AUTH_METHOD:
    case property_name::AUTH_DATA:
    case property_name::RESP_INFO:
    case property_name::SERV_REF:
    case property_name::REASON:
        m_data = parse_binary(data);
        break;
    case property_name::SUB_ID:
        m_data = parse_varint(data);
        break;
    case property_name::USER_PROPERTY:
        m_data = parse_string_pair(data);
        break;
    }
}

uint8_t mqtt::property::as_byte() const {
    return m_data[0];
}

uint16_t mqtt::property::as_twobyte() const {
    return (m_data[0] << 8) | m_data[1];
}

uint32_t mqtt::property::as_fourbyte() const {
    return (m_data[0] << 24) | (m_data[1] << 16) | (m_data[2] << 8) | m_data[3];
}

mqtt::varint_t mqtt::property::as_varint() const {
    return mqtt::varint_t(m_data);
}

std::u8string_view mqtt::property::as_string() const {
    return std::u8string_view{(char8_t*)m_data.data() + 2, m_data.size() - 2};
}

std::pair<std::u8string_view, std::u8string_view> mqtt::property::as_string_pair() const {
    uint16_t len1 = (m_data[0] << 8) | m_data[1];
    uint16_t len2 = (m_data[len1 + 2] << 8) | m_data[len1 + 3];
    std::u8string_view str1{(char8_t*)m_data.subspan(2, len1).data(), len1};
    std::u8string_view str2{(char8_t*)m_data.subspan(len1 + 4, len2).data(), len2};
    return {str1, str2};
}

bool mqtt::property::is_byte() const {
    switch(m_name) {
    case property_name::PAYLOAD_FMT:
    case property_name::REQ_PROB_INFO:
    case property_name::REQ_RESP_INFO:
    case property_name::MAX_QOS:
    case property_name::RETAIN_AVAIL:
    case property_name::WILD_SUB_AVAIL:
    case property_name::SUB_ID_AVAIL:
    case property_name::SHARED_SUB_AVAIL:
        return true;
    default:
        return false;
    }
}

bool mqtt::property::is_twobyte() const {
    switch(m_name) {
    case property_name::KEEP_ALIVE:
    case property_name::RECV_MAX:
    case property_name::TOPIC_ALIAS_MAX:
    case property_name::TOPIC_ALIAS:
        return true;
    default:
        return false;
    }

}

bool mqtt::property::is_fourbyte() const {
    switch(m_name) {
    case property_name::MSG_EXPIRY:
    case property_name::SESS_EXPIRY:
    case property_name::WILL_DELAY:
    case property_name::MAX_PKT_SIZE:
        return true;
    default:
        return false;
    }
}

bool mqtt::property::is_varint() const {
    switch(m_name) {
    case property_name::SUB_ID:
        return true;
    default:
        return false;
    }

}

bool mqtt::property::is_string() const {
    switch(m_name) {
    case property_name::CONTENT_TYPE:
    case property_name::RESP_TOPIC:
    case property_name::CLIENT_ID:
    case property_name::AUTH_METHOD:
    case property_name::RESP_INFO:
    case property_name::SERV_REF:
    case property_name::REASON:
        return true;
    default:
        return false;
    }
}

bool mqtt::property::is_binary() const {
    switch(m_name) {
    case property_name::CORR_DATA:
    case property_name::AUTH_DATA:
        return true;
    default:
        return false;
    }
}

bool mqtt::property::is_string_pair() const {
    switch(m_name) {
    case property_name::USER_PROPERTY:
        return true;
    default:
        return false;
    }

}

uint32_t mqtt::property::size() const {
    return 1 + m_data.size();
}

void mqtt::property::serialize(std::span<uint8_t> value) const {
    value[0] = (uint8_t)m_name;
    memcpy(value.data() + 1, m_data.data(), m_data.size());
}

mqtt::properties::properties(std::span<uint8_t> data) {
    m_length = varint_t(data);
    data = data.subspan(m_length.length);
    uint32_t i = 0;
    m_count = 0;
    m_capacity = 4;
    m_properties = (property*)malloc(sizeof(property) * m_capacity);
    while(i < m_length) {
        if(m_count >= m_capacity) {
            m_capacity *= 2;
            m_properties = (property*)realloc(m_properties, sizeof(property) * m_capacity);
        }
        m_properties[m_count] = property((property_name)data[i], data.subspan(i + 1));
        i += m_properties[m_count].size();
        m_count++;
    }
}

mqtt::properties::~properties() {
    if(m_properties) {
        free(m_properties);
        m_properties = nullptr;
    }
}

void mqtt::properties::serialize(std::span<uint8_t> value) const {
    uint32_t offset = m_length.serialize(value);
    for(uint32_t i = 0; i < m_count; i++) {
        m_properties[i].serialize(value.subspan(offset));
        offset += m_properties[i].size();
    }
}

void mqtt::properties::push_back(property& value) {
    if(m_properties == nullptr) {
        m_capacity = 4;
        m_properties = (property*)malloc(sizeof(property) * m_capacity);
    }
    if(m_count >= m_capacity) {
        m_capacity *= 2;
        m_properties = (property*)realloc(m_properties, sizeof(property) * m_capacity);
    }
    m_properties[m_count] = value;
    m_length.value += m_properties[m_count].size();
    m_count++;
}

void mqtt::properties::pop(uint32_t index) {
    if(m_count == 0) {
        return;
    }
    if(index < m_count - 1) {
        std::swap(m_properties[index], m_properties[m_count - 1]);
    }
    m_count--;
    m_length.value -= m_properties[m_count].size();
    if(m_count < (m_capacity / 2)) {
        m_capacity = m_capacity / 2;
        m_properties = (property*)realloc(m_properties, sizeof(property) * m_capacity);
    }
}

std::optional<const mqtt::property> mqtt::properties::operator[](property_name name) const {
    for(int i = 0; i < m_count; i++) {
        if(m_properties[i].m_name == name) {
            return m_properties[i];
        }
    }
    return {};
}

// mqtt::packet::packet_data(std::span<uint8_t> value) {
//     data = (uint8_t*)malloc(value.size());
//     if(data == nullptr) {
//         panic("mqtt::packet_data constructor: out of memory when allocating data!\n");
//     }
//     std::memcpy(data, value.data(), value.size());
//     m_data = {data, value.size()};
//     m_count = value.size();
//     m_capacity = value.size();
// }

// mqtt::packet_data::~packet_data() {
//     if(data) {
//         free(data);
//         data = nullptr;
//     }
// }

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
    expand_if_needed(value.size());
    memcpy(m_data.data() + m_count, value.data(), value.size());
    m_count += value.size();
    return (*this);
}

mqtt::packet& mqtt::packet::operator+=(const std::span<uint8_t>& value) {
    expand_if_needed(value.size());
    memcpy(m_data.data() + m_count, value.data(), value.size());
    m_count += value.size();
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

mqtt::connect_packet::connect_packet(std::u8string username, std::span<uint8_t> password) : m_will_properties(nullptr) {
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::CONNECT);

    packet& connect = *m_packet;
    connect += u8"MQTT";
    connect += uint8_t{5};

    // flags
    connect += uint8_t{((int)(username.size() > 0) << 7) | ((int)(password.size() > 0) << 6)};
    // keep alive
    connect += uint16_t{0};
    // Empty properties
    m_properties = mqtt::properties();
    connect += m_properties;
    // Client id
    connect += u8"";
    // Add username and password if present
    if(username.size() > 0) {
        connect += username;
    }
    if(password.size() > 0) {
        connect += password;
    }
}

mqtt::connect_packet::connect_packet(mqtt::packet* packet) : m_owned(false), m_packet(packet) {
    m_properties = properties{m_packet->contents().subspan(props_offset())};
    m_will_properties = nullptr;
    if(flags().will()) {
        m_will_properties = new properties(m_packet->contents().subspan(will_props_offset()));
    }
}

mqtt::connect_packet::~connect_packet() {
    if(m_owned && m_packet) {
        delete m_packet;
    }

    if(m_will_properties) {
        delete m_will_properties;
    }
}

size_t mqtt::connect_packet::protocol_offset() const {
    return 0;
}

size_t mqtt::connect_packet::version_offset() const {
    return 6;
}

size_t mqtt::connect_packet::flags_offset() const {
    return 7;
}

size_t mqtt::connect_packet::keep_alive_offset() const {
    return 8;
}

size_t mqtt::connect_packet::props_offset() const {
    return 10;
}

size_t mqtt::connect_packet::client_id_offset() const {
    const properties& props = connect_properties();
    return props_offset() + props.m_length + props.m_length.length;
}

size_t mqtt::connect_packet::will_props_offset() const {
    if(!flags().will()) {
        return 0;
    }
    return client_id_offset() + client_id().size() + 2;
}

size_t mqtt::connect_packet::will_topic_offset() const {
    if(!flags().will()) {
        return 0;
    }
    const properties* will_props = will_properties();
    return will_props_offset() + will_props->m_length + will_props->m_length.length;
}

size_t mqtt::connect_packet::will_payload_offset() const {
    if(!flags().will()) {
        return 0;
    }
    return will_topic_offset() + will_topic()->size() + 2;
}

size_t mqtt::connect_packet::username_offset() const {
    if(!flags().username()) {
        return 0;
    }
    return flags().will() 
        ? (will_payload_offset() + will_payload()->size() + 2) 
        : (client_id_offset() + client_id().size() + 2);
}

size_t mqtt::connect_packet::password_offset() const {
    if(!flags().password()) {
        return 0;
    }
    if(flags().username()) {
        return username_offset() + username()->size() + 2;
    } else if(flags().will()) {
        return will_payload_offset() + will_payload()->size() + 2;
    } else {
        return client_id_offset() + client_id().size() + 2;
    }
}

mqtt::packet_type mqtt::connect_packet::type() const {
    if(!m_packet) {
        return mqtt::packet_type::UNDEFINED;
    }
    return m_packet->m_type;
}

mqtt::varint_t mqtt::connect_packet::length() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->size();
}

std::u8string_view mqtt::connect_packet::protocol() const {
    if(!m_packet) {
        return {};
    }
    return {(char8_t*)m_packet->contents().subspan(protocol_offset() + 2, 4).data(), 4};
}

uint8_t mqtt::connect_packet::version() const {
    if(!m_packet) {
        return {0};
    }
    return m_packet->contents()[version_offset()];
}

mqtt::connect_packet::flags_t mqtt::connect_packet::flags() const {
    if(!m_packet) {
        return {0};
    }
    return {m_packet->contents()[flags_offset()]};
}

uint16_t mqtt::connect_packet::keep_alive() const {
    if(!m_packet) {
        return 0;
    }
    return (((uint16_t)m_packet->contents()[keep_alive_offset()]) << 8) | (m_packet->contents()[keep_alive_offset() + 1]);
}

const mqtt::properties& mqtt::connect_packet::connect_properties() const {
    return m_properties;
}

std::u8string_view mqtt::connect_packet::client_id() const {
    if(!m_packet) {
        return {};
    }
    uint16_t len = ((uint16_t)m_packet->contents()[client_id_offset()] << 8) | m_packet->contents()[client_id_offset() + 1];
    return {(char8_t*)m_packet->contents().subspan(client_id_offset() + 2).data(), len};
}

const mqtt::properties* mqtt::connect_packet::will_properties() const {
    return m_will_properties;
}

std::optional<std::u8string_view> mqtt::connect_packet::will_topic() const {
    if(!m_packet || !flags().will()) {
        return {};
    }
    uint16_t len = ((uint16_t)m_packet->contents()[will_topic_offset()] << 8) | m_packet->contents()[will_topic_offset() + 1];
    return std::u8string_view{(char8_t*)m_packet->contents().subspan(will_topic_offset() + 2).data(), len};
}

std::optional<std::span<uint8_t>> mqtt::connect_packet::will_payload() const {
    if(!m_packet || !flags().will()) {
        return {};
    }
    uint16_t len = ((uint16_t)m_packet->contents()[will_payload_offset()] << 8) | m_packet->contents()[will_payload_offset() + 1];
    return m_packet->contents().subspan(will_payload_offset() + 2, len);
}

std::optional<std::u8string_view> mqtt::connect_packet::username() const {
    if(!m_packet || !flags().username()) {
        return {};
    }
    uint16_t len = ((uint16_t)m_packet->contents()[username_offset()] << 8) | m_packet->contents()[username_offset() + 1];
    return std::u8string_view{(char8_t*)m_packet->contents().subspan(username_offset() + 2).data(), len};
}

std::optional<std::span<uint8_t>> mqtt::connect_packet::password() const {
    if(!m_packet || !flags().password()) {
        return {};
    }
    uint16_t len = ((uint16_t)m_packet->contents()[password_offset()] << 8) | m_packet->contents()[password_offset() + 1];
    return m_packet->contents().subspan(password_offset() + 2, len);
}

std::span<uint8_t> mqtt::connect_packet::serialize() const {
    if(!m_packet) {
        return {};
    }
    return m_packet->serialize();
}

mqtt::packet* mqtt::connect_packet::release() {
    mqtt::packet* to_return = m_packet;
    m_packet = nullptr;
    if(m_will_properties) {
        delete m_will_properties;
        m_will_properties = nullptr;
    }
    return to_return;
}