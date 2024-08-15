#pragma once
#include <mqtt/properties.h>
#include <mqtt/varint.h>
#include <mqtt/packet/base.h>

namespace mqtt {
    struct pingreq_packet {
        pingreq_packet();
        pingreq_packet(packet* p);
        ~pingreq_packet();

        // Fixed header
        packet_type type() const;
        varint_t length() const;

        std::span<uint8_t> serialize() const;

        packet* release();

    private:
        packet* m_packet;
        bool m_owned;
    };
}