#pragma once
#include <mqtt/properties.h>
#include <mqtt/varint.h>
#include <mqtt/packet/base.h>

namespace mqtt {
    struct pubrel_packet {
        pubrel_packet(uint16_t pkt_id, reason_code reason, mqtt::properties properties = mqtt::properties{});
        pubrel_packet(packet* p);
        ~pubrel_packet();

        // Fixed header
        packet_type type() const;
        varint_t length() const;

        // Variable header
        uint16_t id() const;
        reason_code reason() const;
        const mqtt::properties& properties() const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        mqtt::properties m_properties;
        bool m_owned;

        size_t id_offset() const;
        size_t reason_offset() const;
        size_t props_offset() const;
    };
}