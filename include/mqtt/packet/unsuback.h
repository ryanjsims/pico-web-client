#pragma once
#include <mqtt/properties.h>
#include <mqtt/varint.h>
#include <mqtt/packet/base.h>

namespace mqtt {
    struct unsuback_packet {
        unsuback_packet(uint16_t pkt_id, std::span<reason_code> reasons, mqtt::properties properties = mqtt::properties{});
        unsuback_packet(packet* p);
        ~unsuback_packet();

        // Fixed header
        packet_type type() const;
        varint_t length() const;

        // Variable header
        uint16_t id() const;
        const mqtt::properties& properties() const;

        // Payload
        std::span<mqtt::reason_code> reasons() const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        mqtt::properties m_properties;
        bool m_owned;

        size_t id_offset() const;
        size_t props_offset() const;
        size_t reasons_offset() const;
    };
}