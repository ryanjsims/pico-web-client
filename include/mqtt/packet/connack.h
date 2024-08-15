#pragma once
#include <mqtt/properties.h>
#include <mqtt/varint.h>
#include <mqtt/packet/base.h>

namespace mqtt {
    struct connack_packet {
        struct flags_t {
            uint8_t value;
            operator uint8_t() const { return value; }

            bool session_present() const { return (value & (1 << 0)) != 0; }
            void session_present(bool to_set) { value = (to_set ? (value | (1 << 0)) : (value & ~(1 << 0))); }
        };

        connack_packet(flags_t flags, reason_code reason, mqtt::properties properties = mqtt::properties{});
        connack_packet(packet* p);
        ~connack_packet();

        // Fixed header
        packet_type type() const;
        varint_t length() const;

        // Variable header
        flags_t flags() const;
        reason_code reason() const;
        const mqtt::properties& properties() const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        mqtt::properties m_properties;
        bool m_owned;

        size_t flags_offset() const;
        size_t reason_offset() const;
        size_t props_offset() const;
    };
}