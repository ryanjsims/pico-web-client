#pragma once
#include <span>
#include <cstdint>

namespace mqtt {
    struct varint_t {
        varint_t() = default;
        varint_t(std::span<uint8_t>);
        varint_t(uint32_t);

        uint32_t value;
        uint8_t length;

        uint8_t serialize(std::span<uint8_t>) const;

        operator int() const;
    };
}