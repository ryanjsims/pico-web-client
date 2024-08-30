#pragma once
#include <span>
#include <string>
#include <optional>

#include <cstdint>

#include <mqtt/properties.h>
#include <mqtt/varint.h>

namespace mqtt {
    enum class packet_type : uint8_t {
        UNDEFINED   = 0x0,
        CONNECT     = 0x1 << 4,
        CONNACK     = 0x2 << 4,
        PUBLISH     = 0x3 << 4,
        PUBACK      = 0x4 << 4,
        PUBREC      = 0x5 << 4,
        PUBREL      = 0x62,
        PUBCOMP     = 0x7 << 4,
        SUBSCRIBE   = 0x82,
        SUBACK      = 0x9 << 4,
        UNSUBSCRIBE = 0xA2,
        UNSUBACK    = 0xB << 4,
        PINGREQ     = 0xC << 4,
        PINGRESP    = 0xD << 4,
        DISCONNECT  = 0xE << 4,
        AUTH        = 0xF << 4,
        MASK        = 0xF0,
    };

    std::string packet_type_string(packet_type type);

    enum class reason_code : uint8_t {
        SUCCESS                  = 0x00,
        NORMAL_DISCONNECT        = 0x00,
        GRANTED_QoS_0            = 0x00,
        GRANTED_QoS_1            = 0x01,
        GRANTED_QoS_2            = 0x02,
        DISCONNECT_WILL          = 0x04,
        NO_MATCHING_SUBS         = 0x10,
        NO_EXISTING_SUB          = 0x11,
        CONTINUE_AUTH            = 0x18,
        REAUTHENTICATE           = 0x19,
        ERROR_UNSPECIFIED        = 0x80,
        ERROR_MALFORMED_PKT      = 0x81,
        ERROR_PROTOCOL           = 0x82,
        ERROR_IMPL_SPECIFIC      = 0x83,
        ERROR_UNSUP_VERSION      = 0x84,
        ERROR_INV_CLIENT_ID      = 0x85,
        ERROR_BAD_CREDS          = 0x86,
        ERROR_UNAUTHORIZED       = 0x87,
        ERROR_SERV_UNAVAIL       = 0x88,
        ERROR_SERV_BUSY          = 0x89,
        ERROR_BANNED             = 0x8A,
        ERROR_SERV_SHUTDOWN      = 0x8B,
        ERROR_BAD_AUTH_METH      = 0x8C,
        ERROR_TIMEOUT            = 0x8D,
        ERROR_SESS_TAKEN         = 0x8E,
        ERROR_TPC_FIL_INV        = 0x8F,
        ERROR_TPC_NAME_INV       = 0x90,
        ERROR_PKT_ID_USED        = 0x91,
        ERROR_PKT_ID_DNE         = 0x92,
        ERROR_RECV_MAX_EXC       = 0x93,
        ERROR_TPC_ALIAS_INV      = 0x94,
        ERROR_PKT_TOO_LRG        = 0x95,
        ERROR_MSG_RATE_EXC       = 0x96,
        ERROR_QUOTA_EXC          = 0x97,
        ERROR_ADMIN_ACTION       = 0x98,
        ERROR_PAY_FMT_INV        = 0x99,
        ERROR_RET_NOT_SUP        = 0x9A,
        ERROR_QOS_NOT_SUP        = 0x9B,
        ERROR_USE_DIFF_SERV      = 0x9C,
        ERROR_SERV_MOVED         = 0x9D,
        ERROR_SHARED_SUB_NOT_SUP = 0x9E,
        ERROR_CONN_RATE_EXC      = 0x9F,
        ERROR_MAX_CONN_TIME      = 0xA0,
        ERROR_SUB_IDS_NOT_SUP    = 0xA1,
        ERROR_WILD_SUBS_NOT_SUP  = 0xA2,
    };

    const std::string reason_string(reason_code code, packet_type type);

    struct packet {
        packet_type m_type;

        packet() : m_type(packet_type::UNDEFINED), m_length(0), m_count(0), m_capacity(0) {}
        packet(packet_type t);
        packet(std::span<uint8_t>);
        ~packet();

        packet& operator+=(const std::u8string&);
        packet& operator+=(const std::span<uint8_t>&);
        packet& operator+=(uint8_t);
        packet& operator+=(uint16_t);
        packet& operator+=(uint32_t);
        packet& operator+=(varint_t);
        packet& operator+=(const properties&);

        void add_raw(const std::span<uint8_t>&);

        std::span<uint8_t> serialize();
        const std::span<uint8_t> contents();
        uint8_t qos();
        bool dup();
        void dup(bool);
        varint_t size() { return {m_count}; }

        packet_type masked() const {
            mqtt::packet_type to_ret = (mqtt::packet_type)((uint8_t)m_type & (uint8_t)mqtt::packet_type::MASK);
            // Only PUBLISH packets need to be masked since they're the only packet type with
            // variable flags stored in the packet type byte
            if(to_ret == mqtt::packet_type::PUBLISH) {
                return mqtt::packet_type::PUBLISH;
            }
            return m_type;
        }

    private:
        varint_t m_length;
        uint8_t *data = nullptr;
        std::span<uint8_t> m_data;
        uint32_t m_count, m_capacity;
        bool expand_if_needed(uint32_t length_to_add);
    };
};