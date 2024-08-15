#pragma once
#include <mqtt/properties.h>
#include <mqtt/varint.h>
#include <mqtt/packet/base.h>

namespace mqtt {
    struct disconnect_packet {
        disconnect_packet(reason_code reason, mqtt::properties properties = mqtt::properties{});
        disconnect_packet(packet* p);
        ~disconnect_packet();

        // Fixed header
        packet_type type() const;
        varint_t length() const;

        // Variable header
        reason_code reason() const;
        const mqtt::properties& properties() const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        mqtt::properties m_properties;
        bool m_owned;

        size_t reason_offset() const;
        size_t props_offset() const;
    };
}