#include <mqtt/properties.h>

#include <logger.h>

#include <cstring>

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
    // Varint has minimum size of 1
    uint8_t i = 1;
    while(i < 4 && (data[i] & 0x80)) {
        i++;
    }
    return data.first(i);
}

std::span<uint8_t> mqtt::property::parse(mqtt::property_name name, std::span<uint8_t> data) {
    switch(name) {
    case property_name::PAYLOAD_FMT:
    case property_name::REQ_PROB_INFO:
    case property_name::REQ_RESP_INFO:
    case property_name::MAX_QOS:
    case property_name::RETAIN_AVAIL:
    case property_name::WILD_SUB_AVAIL:
    case property_name::SUB_ID_AVAIL:
    case property_name::SHARED_SUB_AVAIL:
        return data.first(1);
    case property_name::KEEP_ALIVE:
    case property_name::RECV_MAX:
    case property_name::TOPIC_ALIAS_MAX:
    case property_name::TOPIC_ALIAS:
        return data.first(2);
    case property_name::MSG_EXPIRY:
    case property_name::SESS_EXPIRY:
    case property_name::WILL_DELAY:
    case property_name::MAX_PKT_SIZE:
        return data.first(4);
    case property_name::CONTENT_TYPE:
    case property_name::RESP_TOPIC:
    case property_name::CORR_DATA:
    case property_name::CLIENT_ID:
    case property_name::AUTH_METHOD:
    case property_name::AUTH_DATA:
    case property_name::RESP_INFO:
    case property_name::SERV_REF:
    case property_name::REASON:
        return parse_binary(data);
    case property_name::SUB_ID:
        return parse_varint(data);
    case property_name::USER_PROPERTY:
        return parse_string_pair(data);
    default:
        error("mqtt::property::parse: unknown property 0x%02x\n", name);
        return {};
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
    uint16_t length = (uint16_t)((m_data[0] << 8) | m_data[1]);
    return std::u8string_view{(char8_t*)m_data.data() + 2, length};
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

mqtt::properties::properties(std::span<uint8_t> data): m_properties({}) {
    if(data.size() == 0) {
        m_length = {0};
        return;
    }
    m_length = varint_t(data);
    data = data.subspan(m_length.length);
    uint32_t i = 0;
    while(i < m_length) {
        property_name name = (property_name)data[i];

        m_properties.push_back({name, property::parse(name, data.subspan(i+1))});
        i += m_properties[m_properties.size() - 1].size();
    }
}

mqtt::properties::properties(mqtt::properties&& other) {
    m_properties = other.m_properties;
    m_length = other.m_length;
    other.m_properties = {};
    other.m_length = {0};
}

void mqtt::properties::serialize(std::span<uint8_t> value) const {
    uint32_t offset = m_length.serialize(value);
    for(uint32_t i = 0; i < m_properties.size(); i++) {
        m_properties[i].serialize(value.subspan(offset));
        offset += m_properties[i].size();
    }
}

void mqtt::properties::push_back(property& value) {
    m_properties.push_back(value);
    m_length.value += m_properties[m_properties.size() - 1].size();
}

void mqtt::properties::pop(uint32_t index) {
    if(m_properties.size() == 0 || index >= m_properties.size()) {
        return;
    }
    m_length.value -= m_properties[index].size();
    m_properties.erase(m_properties.begin() + index);
}

std::vector<const mqtt::property*> mqtt::properties::operator[](property_name name) const {
    std::vector<const mqtt::property*> to_return;
    for(int i = 0; i < m_properties.size(); i++) {
        if(m_properties[i].m_name == name) {
            to_return.push_back(&m_properties[i]);
        }
    }
    return to_return;
}