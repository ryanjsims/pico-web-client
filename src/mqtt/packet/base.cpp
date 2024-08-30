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

const std::string mqtt::reason_string(mqtt::reason_code code, mqtt::packet_type type) {
    switch(type) {
        case packet_type::CONNACK:
            switch(code){
            case reason_code::SUCCESS:
                return "Success";
            case reason_code::ERROR_UNSPECIFIED:
                return "Unspecified error";
            case reason_code::ERROR_MALFORMED_PKT:
                return "Malformed Packet";
            case reason_code::ERROR_PROTOCOL:
                return "Protocol Error";
            case reason_code::ERROR_IMPL_SPECIFIC:
                return "Implementation specific error";
            case reason_code::ERROR_UNSUP_VERSION:
                return "Unsupported Protocol Version";
            case reason_code::ERROR_INV_CLIENT_ID:
                return "Client Identifier not valid";
            case reason_code::ERROR_BAD_CREDS:
                return "Bad User Name or Password";
            case reason_code::ERROR_UNAUTHORIZED:
                return "Not authorized";
            case reason_code::ERROR_SERV_UNAVAIL:
                return "Server unavailable";
            case reason_code::ERROR_SERV_BUSY:
                return "Server busy";
            case reason_code::ERROR_BANNED:
                return "Banned";
            case reason_code::ERROR_BAD_AUTH_METH:
                return "Bad authentication method";
            case reason_code::ERROR_TPC_NAME_INV:
                return "Topic Name invalid";
            case reason_code::ERROR_PKT_TOO_LRG:
                return "Packet too large";
            case reason_code::ERROR_QUOTA_EXC:
                return "Quota exceeded";
            case reason_code::ERROR_PAY_FMT_INV:
                return "Payload format invalid";
            case reason_code::ERROR_RET_NOT_SUP:
                return "Retain not supported";
            case reason_code::ERROR_QOS_NOT_SUP:
                return "QoS not supported";
            case reason_code::ERROR_USE_DIFF_SERV:
                return "Use another server";
            case reason_code::ERROR_SERV_MOVED:
                return "Server moved";
            case reason_code::ERROR_CONN_RATE_EXC:
                return "Connection rate exceeded";
            }
        case packet_type::PUBACK:
        case packet_type::PUBREC:
            switch(code) {
            case reason_code::SUCCESS:
                return "Success";
            case reason_code::NO_MATCHING_SUBS:
                return "No matching subscribers";
            case reason_code::ERROR_UNSPECIFIED:
                return "Unspecified error";
            case reason_code::ERROR_IMPL_SPECIFIC:
                return "Implementation specific error";
            case reason_code::ERROR_UNAUTHORIZED:
                return "Not authorized";
            case reason_code::ERROR_TPC_NAME_INV:
                return "Topic Name invalid";
            case reason_code::ERROR_PKT_ID_USED:
                return "Packet Identifier in use";
            case reason_code::ERROR_QUOTA_EXC:
                return "Quota exceeded";
            case reason_code::ERROR_PAY_FMT_INV:
                return "Payload format invalid";
            }
        case packet_type::PUBREL:
        case packet_type::PUBCOMP:
            switch(code) {
            case reason_code::SUCCESS:
                return "Success";
            case reason_code::ERROR_PKT_ID_DNE:
                return "Packet Identifier not found";
            }
        case packet_type::SUBACK:
            switch(code) {
            case reason_code::GRANTED_QoS_0:
                return "Granted QoS 0";
            case reason_code::GRANTED_QoS_1:
                return "Granted QoS 1";
            case reason_code::GRANTED_QoS_2:
                return "Granted QoS 2";
            case reason_code::ERROR_UNSPECIFIED:
                return "Unspecified error";
            case reason_code::ERROR_IMPL_SPECIFIC:
                return "Implementation specific error";
            case reason_code::ERROR_UNAUTHORIZED:
                return "Not authorized";
            case reason_code::ERROR_TPC_FIL_INV:
                return "Topic Filter invalid";
            case reason_code::ERROR_PKT_ID_USED:
                return "Packet Identifier in use";
            case reason_code::ERROR_QUOTA_EXC:
                return "Quota exceeded";
            case reason_code::ERROR_SHARED_SUB_NOT_SUP:
                return "Shared Subscriptions not supported";
            case reason_code::ERROR_SUB_IDS_NOT_SUP:
                return "Subscription Identifiers not supported";
            case reason_code::ERROR_WILD_SUBS_NOT_SUP:
                return "Wildcard Subscriptions not supported";
            }
        case packet_type::UNSUBACK:
            switch(code) {
            case reason_code::SUCCESS:
                return "Success";
            case reason_code::NO_EXISTING_SUB:
                return "No subscription existed";
            case reason_code::ERROR_UNSPECIFIED:
                return "Unspecified error";
            case reason_code::ERROR_IMPL_SPECIFIC:
                return "Implementation specific error";
            case reason_code::ERROR_UNAUTHORIZED:
                return "Not authorized";
            case reason_code::ERROR_TPC_FIL_INV:
                return "Topic Filter invalid";
            case reason_code::ERROR_PKT_ID_USED:
                return "Packet Identifier in use";
            }
        case packet_type::DISCONNECT:
            switch(code) {
            case reason_code::NORMAL_DISCONNECT:
                return "Normal disconnection";
            case reason_code::DISCONNECT_WILL:
                return "Disconnect with Will Message";
            case reason_code::ERROR_UNSPECIFIED:
                return "Unspecified error";
            case reason_code::ERROR_MALFORMED_PKT:
                return "Malformed Packet";
            case reason_code::ERROR_PROTOCOL:
                return "Protocol Error";
            case reason_code::ERROR_IMPL_SPECIFIC:
                return "Implementation specific error";
            case reason_code::ERROR_UNAUTHORIZED:
                return "Not authorized";
            case reason_code::ERROR_SERV_BUSY:
                return "Server busy";
            case reason_code::ERROR_SERV_SHUTDOWN:
                return "Server shutting down";
            case reason_code::ERROR_BAD_AUTH_METH:
                return "Bad authentication method";
            case reason_code::ERROR_TIMEOUT:
                return "Keep Alive timeout";
            case reason_code::ERROR_SESS_TAKEN:
                return "Session taken over";
            case reason_code::ERROR_TPC_FIL_INV:
                return "Topic Filter invalid";
            case reason_code::ERROR_TPC_NAME_INV:
                return "Topic Name invalid";
            case reason_code::ERROR_RECV_MAX_EXC:
                return "Receive Maximum exceeded";
            case reason_code::ERROR_TPC_ALIAS_INV:
                return "Topic Alias invalid";
            case reason_code::ERROR_PKT_TOO_LRG:
                return "Packet too large";
            case reason_code::ERROR_MSG_RATE_EXC:
                return "Message rate too high";
            case reason_code::ERROR_QUOTA_EXC:
                return "Quota exceeded";
            case reason_code::ERROR_ADMIN_ACTION:
                return "Administrative action";
            case reason_code::ERROR_PAY_FMT_INV:
                return "Payload format invalid";
            case reason_code::ERROR_RET_NOT_SUP:
                return "Retain not supported";
            case reason_code::ERROR_QOS_NOT_SUP:
                return "QoS not supported";
            case reason_code::ERROR_USE_DIFF_SERV:
                return "Use another server";
            case reason_code::ERROR_SERV_MOVED:
                return "Server moved";
            case reason_code::ERROR_SHARED_SUB_NOT_SUP:
                return "Shared Subscriptions not supported";
            case reason_code::ERROR_CONN_RATE_EXC:
                return "Connection rate exceeded";
            case reason_code::ERROR_MAX_CONN_TIME:
                return "Maximum connect time";
            case reason_code::ERROR_SUB_IDS_NOT_SUP:
                return "Subscription Identifiers not supported";
            case reason_code::ERROR_WILD_SUBS_NOT_SUP:
                return "Wildcard Subscriptions not supported";
            }
        case packet_type::AUTH:
            switch(code) {
            case reason_code::SUCCESS:
                return "Success";
            case reason_code::CONTINUE_AUTH:
                return "Continue authentication";
            case reason_code::REAUTHENTICATE:
                return "Re-authenticate";
            }
        default:
            return "(unknown)";
    }
}

mqtt::packet::packet(packet_type t) : m_type(t), m_length(0), m_count(0), m_capacity(32) {
    data = (uint8_t*)malloc(m_capacity + 5);
    if(data == nullptr) {
        panic("mqtt::packet constructor: OOM when constructing minimal packet\n");
    }
    m_data = {data + 5, (uint32_t)m_capacity};
}

mqtt::packet::packet(std::span<uint8_t> value) {
    m_type = (packet_type)value[0];
    m_length = varint_t{value.subspan(1)};
    debug("mqtt::packet constructor: Creating %.*s packet of size %d\n", packet_type_string(masked()).size(), packet_type_string(masked()).data(), m_length + 5);
    data = (uint8_t*)malloc(m_length + 5);
    if(data == nullptr) {
        panic("mqtt::packet constructor: OOM when constructing packet\n");
    }
    memcpy(data + 5, value.data() + 1 + m_length.length, m_length);
    m_data = {data + 5, (uint32_t)m_length};
    m_capacity = m_length;
    m_count = m_length;
}

mqtt::packet::~packet() {
    debug("mqtt::packet destructor: Deleting %.*s packet\n", packet_type_string(masked()).size(), packet_type_string(masked()).data());
    if(data) {
        free(data);
        data = nullptr;
        m_count = 0;
        m_data = {};
    }
}

bool mqtt::packet::expand_if_needed(uint32_t length_to_add) {
    if(data == nullptr) {
        m_capacity = 128;
        data = (uint8_t*)malloc(m_capacity);
        if(data == nullptr) {
            error1("mqtt::packet::expand_if_needed: Failed to allocate data\n");
            m_data = {};
            return false;
        }
        trace("mqtt::packet::expand_if_needed: Created data %p with capacity 128\n", data);
    } else if((m_count + length_to_add) > m_capacity) {
        m_capacity = MAX(2 * m_capacity, m_count + length_to_add + 32);
        uint8_t* new_data = (uint8_t*)realloc(data, m_capacity);
        if(new_data == nullptr) {
            error1("mqtt::packet::expand_if_needed: Failed to reallocate data\n");
            return false;
        }
        data = new_data;
        trace("mqtt::packet::expand_if_needed: Reallocated data %p with capacity %d\n", data, m_capacity);
    }
    m_data = {data + 5, m_capacity};
    return true;
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

std::span<uint8_t> mqtt::packet::serialize() {
    m_length = varint_t(m_count);
    m_length.serialize({data + 5 - m_length.length, m_length.length});
    data[4 - m_length.length] = (uint8_t)m_type;
    return {data + 4 - m_length.length, m_length + 1 + m_length.length};
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

bool mqtt::packet::dup() {
    if(masked() == mqtt::packet_type::PUBLISH) {
        return (uint8_t)m_type & (1 << 3);
    }
    return false;
}

void mqtt::packet::dup(bool to_set) {
    if(masked() == mqtt::packet_type::PUBLISH) {
        m_type = (mqtt::packet_type)(to_set ? ((uint8_t)m_type | (1 << 3)) : ((uint8_t)m_type & ~(1 << 3)));
    }
}
