#pragma once
#include <mqtt/properties.h>
#include <mqtt/varint.h>
#include <mqtt/packet/base.h>
#include <vector>

namespace mqtt {
    struct unsubscribe_packet {
        unsubscribe_packet(uint16_t pkt_id, std::u8string topic_filter, mqtt::properties properties = mqtt::properties{});
        unsubscribe_packet(uint16_t pkt_id, std::vector<std::u8string> topic_filters, mqtt::properties properties = mqtt::properties{});
        unsubscribe_packet(packet* p);
        ~unsubscribe_packet();

        // Fixed header
        packet_type type() const;
        varint_t length() const;

        // Variable header
        uint16_t id() const;
        const mqtt::properties& properties() const;

        std::u8string_view topic_filter(uint index) const;

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