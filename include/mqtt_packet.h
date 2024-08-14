#pragma once
#include <span>
#include <string>
#include <optional>

#include <cstdint>

namespace mqtt {
    enum class packet_type : uint8_t {
        UNDEFINED   = 0x0,
        CONNECT     = 0x1 << 4,
        CONNACK     = 0x2 << 4,
        PUBLISH     = 0x3 << 4,
        PUBACK      = 0x4 << 4,
        PUBREC      = 0x5 << 4,
        PUBREL      = 0x6 << 4,
        PUBCOMP     = 0x7 << 4,
        SUBSCRIBE   = 0x8 << 4,
        SUBACK      = 0x9 << 4,
        UNSUBSCRIBE = 0xA << 4,
        UNSUBACK    = 0xB << 4,
        PINGREQ     = 0xC << 4,
        PINGRESP    = 0xD << 4,
        DISCONNECT  = 0xE << 4,
        AUTH        = 0xF << 4,
        MASK        = 0xF0,
    };

    enum class packet_flags : uint8_t {
        PUBLISH_DUP = 0x1 << 3,
        PUBLISH_QoS = 0x3 << 1,
        PUBLISH_RET = 0x1 << 0,
        MASK        = 0x0F,
    };

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

    enum class property_name : uint8_t {
        PAYLOAD_FMT      = 0x01,
        MSG_EXPIRY       = 0x02,
        CONTENT_TYPE     = 0x03,
        RESP_TOPIC       = 0x08,
        CORR_DATA        = 0x09,
        SUB_ID           = 0x0B,
        SESS_EXPIRY      = 0x11,
        CLIENT_ID        = 0x12,
        KEEP_ALIVE       = 0x13,
        AUTH_METHOD      = 0x15,
        AUTH_DATA        = 0x16,
        REQ_PROB_INFO    = 0x17,
        WILL_DELAY       = 0x18,
        REQ_RESP_INFO    = 0x19,
        RESP_INFO        = 0x1A,
        SERV_REF         = 0x1C,
        REASON           = 0x1F,
        RECV_MAX         = 0x21,
        TOPIC_ALIAS_MAX  = 0x22,
        TOPIC_ALIAS      = 0x23,
        MAX_QOS          = 0x24,
        RETAIN_AVAIL     = 0x25,
        USER_PROPERTY    = 0x26,
        MAX_PKT_SIZE     = 0x27,
        WILD_SUB_AVAIL   = 0x28,
        SUB_ID_AVAIL     = 0x29,
        SHARED_SUB_AVAIL = 0x2A,
        __VALUE_MAX
    };

    std::span<uint8_t> parse_binary(std::span<uint8_t> data);
    std::span<uint8_t> parse_string_pair(std::span<uint8_t> data);
    std::span<uint8_t> parse_varint(std::span<uint8_t> data);

    struct varint_t {
        varint_t() = default;
        varint_t(std::span<uint8_t>);
        varint_t(uint32_t);

        uint32_t value;
        uint8_t length;

        uint8_t serialize(std::span<uint8_t>) const;

        operator int() const;
    };

    struct property {
        property(property_name name, std::span<uint8_t> data);
        property_name m_name;
        std::span<uint8_t> m_data;

        uint8_t as_byte() const;
        bool is_byte() const;
        uint16_t as_twobyte() const;
        bool is_twobyte() const;
        uint32_t as_fourbyte() const;
        bool is_fourbyte() const;
        varint_t as_varint() const;
        bool is_varint() const;
        std::u8string_view as_string() const;
        bool is_string() const;
        std::pair<std::u8string_view, std::u8string_view> as_string_pair() const;
        bool is_string_pair() const;

        bool is_binary() const;

        uint32_t size() const;
        void serialize(std::span<uint8_t>) const;
    };

    struct properties {
        varint_t m_length;

        properties() : m_length(0) {};
        properties(std::span<uint8_t>);
        ~properties();

        void push_back(property&);
        void pop(uint32_t);

        void serialize(std::span<uint8_t>) const;

        std::optional<const property> operator[](property_name name) const;

    private:
        property *m_properties = nullptr;
        uint32_t m_count = 0, m_capacity = 0;
    };

    struct packet {
        packet() : m_type(packet_type::UNDEFINED), m_length(0) {}
        packet(packet_type t) : m_type(t), m_length(0) {}
        packet(std::span<uint8_t>);
        ~packet();

        packet& operator+=(const std::u8string&);
        packet& operator+=(const std::span<uint8_t>&);
        packet& operator+=(uint8_t);
        packet& operator+=(uint16_t);
        packet& operator+=(uint32_t);
        packet& operator+=(varint_t);
        packet& operator+=(const properties&);

        std::span<uint8_t> serialize();
        const std::span<uint8_t> contents();
        uint8_t qos();
        varint_t size() { return {m_count}; }

        packet_type m_type;
    private:
        varint_t m_length;
        uint8_t *data = nullptr;
        std::span<uint8_t> m_data;
        uint32_t m_count, m_capacity;
        void expand_if_needed(uint32_t length_to_add);
    };

    struct connect_packet {
        struct flags_t {
            uint8_t value;
            operator uint8_t() const { return value; }

            bool reserved() const { return (value & 1) != 0; }
            //void reserved(bool to_set) { value = (to_set ? (value | (1 << 0)) : (value & ~(1 << 0))); }

            bool clean_start() const { return (value & (1 << 1)) != 0; }
            void clean_start(bool to_set) { value = (to_set ? (value | (1 << 1)) : (value & ~(1 << 1))); }

            bool will() const { return (value & (1 << 2)) != 0; }
            void will(bool to_set) { value = (to_set ? (value | (1 << 2)) : (value & ~(1 << 2))); }

            uint8_t will_qos() const { return ((value >> 3) & 3); }
            void will_qos(uint8_t to_set) {
                // prevent malformed packet
                if((to_set & 3) == 3) {
                    to_set = 2;
                }
                value = (value & ~(3 << 3)) | ((to_set & 3) << 3);
            }

            bool will_ret() const { return (value & (1 << 5)) != 0; }
            void will_ret(bool to_set) { value = (to_set ? (value | (1 << 5)) : (value & ~(1 << 5))); }

            bool password() const { return (value & (1 << 6)) != 0; }
            void password(bool to_set) { value = (to_set ? (value | (1 << 6)) : (value & ~(1 << 6))); }

            bool username() const { return (value & (1 << 7)) != 0; }
            void username(bool to_set) { value = (to_set ? (value | (1 << 7)) : (value & ~(1 << 7))); }
        };

        connect_packet(std::u8string username = u8"", std::span<uint8_t> password = {});
        connect_packet(packet* p);
        ~connect_packet();

        // Fixed header
        packet_type type() const;
        varint_t length() const;

        // Variable header
        std::u8string_view protocol() const;
        uint8_t version() const;
        flags_t flags() const;
        uint16_t keep_alive() const;
        const properties& connect_properties() const;

        // Payload
        std::u8string_view client_id() const;
        const properties* will_properties() const;
        std::optional<std::u8string_view> will_topic() const;
        std::optional<std::span<uint8_t>> will_payload() const;
        std::optional<std::u8string_view> username() const;
        std::optional<std::span<uint8_t>> password() const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        properties m_properties;
        properties *m_will_properties;
        bool m_owned;

        size_t protocol_offset() const;
        size_t version_offset() const;
        size_t flags_offset() const;
        size_t keep_alive_offset() const;
        size_t props_offset() const;
        size_t client_id_offset() const;
        size_t will_props_offset() const;
        size_t will_topic_offset() const;
        size_t will_payload_offset() const;
        size_t username_offset() const;
        size_t password_offset() const;
    };

    
};