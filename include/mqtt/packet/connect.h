#pragma once
#include <mqtt/properties.h>
#include <mqtt/varint.h>
#include <mqtt/packet/base.h>

namespace mqtt {
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

        connect_packet(
            std::u8string client_id = u8"",
            flags_t flags = {},
            uint16_t keep_alive = 0,
            mqtt::properties properties = mqtt::properties{},
            mqtt::properties will_props = mqtt::properties{},
            std::u8string will_topic = u8"",
            std::span<uint8_t> will_payload = {},
            std::u8string username = u8"",
            std::span<uint8_t> password = {}
        );
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
        const properties& will_properties() const;
        std::optional<std::u8string_view> will_topic() const;
        std::optional<std::span<uint8_t>> will_payload() const;
        std::optional<std::u8string_view> username() const;
        std::optional<std::span<uint8_t>> password() const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        properties m_properties;
        properties m_will_properties;
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
}