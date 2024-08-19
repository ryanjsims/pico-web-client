#pragma once
#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include <mqtt/varint.h>

namespace mqtt {
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

    const std::string property_string(property_name name);

    std::span<uint8_t> parse_binary(std::span<uint8_t> data);
    std::span<uint8_t> parse_string_pair(std::span<uint8_t> data);
    std::span<uint8_t> parse_varint(std::span<uint8_t> data);

    struct property {
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
        static std::span<uint8_t> parse(property_name name, std::span<uint8_t> data);
    };

    struct properties {
        varint_t m_length;

        properties() : m_length(0) {};
        properties(std::span<uint8_t>);
        properties(properties&) = default;
        properties(properties&&);

        void push_back(property&);
        void pop(uint32_t);

        void serialize(std::span<uint8_t>) const;

        std::vector<const property*> operator[](property_name name) const;

    private:
        std::vector<property> m_properties;
    };
}