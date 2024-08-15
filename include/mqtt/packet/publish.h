#pragma once
#include <mqtt/varint.h>
#include <mqtt/properties.h>
#include <mqtt/packet/base.h>

namespace mqtt {
    struct publish_packet {
        struct flags_t {
            uint8_t value;
            operator uint8_t() const { return value; }

            bool retain() const { return (value & (1 << 0)) != 0; }
            void retain(bool to_set) { value = (to_set ? (value | (1 << 0)) : (value & ~(1 << 0))); }

            uint8_t qos() const { return (value >> 1) & 0x3; }
            void qos(uint8_t to_set) {
                if((to_set & 0x3) == 3) {
                    to_set = 2;
                }
                value = (value & ~(3 << 1)) | (to_set << 1);
            }

            bool duplicate() const { return (value & (1 << 3)) != 0; }
            void duplicate(bool to_set) { value = (to_set ? (value | (1 << 3)) : (value & ~(1 << 3))); }
        };

        publish_packet(flags_t flags, std::u8string topic_name, uint16_t pkt_id, std::span<uint8_t> payload, mqtt::properties properties = mqtt::properties{});
        publish_packet(packet* p);
        ~publish_packet();

        // Fixed header
        packet_type type() const;
        flags_t flags() const;
        varint_t length() const;

        // Variable header
        std::u8string_view topic() const;
        std::optional<uint16_t> id() const;
        const mqtt::properties& properties() const;

        std::span<uint8_t> payload() const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        mqtt::properties m_properties;
        bool m_owned;

        size_t topic_offset() const;
        size_t topic_length() const;
        size_t id_offset() const;
        size_t props_offset() const;
        size_t payload_offset() const;
    };
}