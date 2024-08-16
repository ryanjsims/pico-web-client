#include <mqtt/packet/connect.h>

mqtt::connect_packet::connect_packet(std::u8string client_id, std::u8string username, std::span<uint8_t> password, mqtt::properties properties)
    : m_will_properties(nullptr)
    , m_properties(std::move(properties))
{
    m_owned = true;
    m_packet = new packet(mqtt::packet_type::CONNECT);

    packet& connect = *m_packet;
    connect += u8"MQTT";
    connect += uint8_t{5};

    // flags
    connect += uint8_t{((int)(username.size() > 0) << 7) | ((int)(password.size() > 0) << 6)};
    // keep alive
    connect += uint16_t{0};
    // properties
    connect += m_properties;
    // Client id
    connect += client_id;
    // Add username and password if present
    if(username.size() > 0) {
        connect += username;
    }
    if(password.size() > 0) {
        connect += password;
    }
}

mqtt::connect_packet::connect_packet(mqtt::packet* packet)
    : m_owned(false)
    , m_packet(packet)
    , m_properties(m_packet->contents().subspan(props_offset()))
{
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
    m_owned = false;
    if(m_will_properties) {
        delete m_will_properties;
        m_will_properties = nullptr;
    }
    return to_return;
}