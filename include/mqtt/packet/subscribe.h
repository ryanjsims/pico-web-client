#pragma once
#include <mqtt/properties.h>
#include <mqtt/varint.h>
#include <mqtt/packet/base.h>

namespace mqtt {
    struct subscribe_packet {
        struct options_t {
            uint8_t value;
            operator uint8_t() const { return value; }
            options_t(uint8_t qos, bool nl, bool rap, uint8_t handling) {
                qos = qos > 2 ? 2 : qos;
                handling = handling > 2 ? 2 : handling;
                value = (handling << 4) | (rap << 3) | (nl << 2) | qos;
            }

            uint8_t qos() const { return ((value >> 0) & 3); }
            void qos(uint8_t to_set) {
                // prevent malformed packet
                if((to_set & 3) == 3) {
                    to_set = 2;
                }
                value = (value & ~(3 << 0)) | ((to_set & 3) << 0);
            }

            bool no_local() const { return (value & (1 << 2)) != 0; }
            void no_local(bool to_set) { value = (to_set ? (value | (1 << 2)) : (value & ~(1 << 2))); }

            bool ret_as_published() const { return (value & (1 << 3)) != 0; }
            void ret_as_published(bool to_set) { value = (to_set ? (value | (1 << 3)) : (value & ~(1 << 3))); }

            uint8_t retain_handling() const { return ((value >> 4) & 3); }
            void retain_handling(uint8_t to_set) {
                // prevent malformed packet
                if((to_set & 3) == 3) {
                    to_set = 2;
                }
                value = (value & ~(3 << 4)) | ((to_set & 3) << 4);
            }
        };
        subscribe_packet(uint16_t pkt_id, std::u8string topic_filter, options_t options, mqtt::properties properties = mqtt::properties{});
        subscribe_packet(uint16_t pkt_id, std::span<std::u8string> topic_filters, std::span<options_t> options, mqtt::properties properties = mqtt::properties{});
        subscribe_packet(packet* p);
        ~subscribe_packet();

        // Fixed header
        packet_type type() const;
        varint_t length() const;

        // Variable header
        uint16_t id() const;
        const mqtt::properties& properties() const;

        std::u8string_view topic_filter(uint index) const;
        std::optional<options_t> options(uint index) const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        mqtt::properties m_properties;
        bool m_owned;

        size_t id_offset() const;
        size_t props_offset() const;
        size_t payload_offset() const;
    };
}