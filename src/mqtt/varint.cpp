#include <mqtt/varint.h>

mqtt::varint_t::varint_t(std::span<uint8_t> data) {
    value = 0;
    length = 0;
    uint8_t shift = 0;
    uint8_t encoded;
    do {
        encoded = data[0];
        value += (encoded & 0x7F) << shift;
        if(shift > 21) {
            value = -1;
            break;
        }
        shift += 7;
        length += 1;
        data = data.subspan(1);
    } while(encoded & 0x80);
}

mqtt::varint_t::varint_t(uint32_t curr_value) : value(curr_value) {
    uint8_t encoded;
    length = 0;
    do {
        curr_value = curr_value >> 7;
        if(length == 4) {
            return;
        }
        length++;
    } while(curr_value > 0);
}

uint8_t mqtt::varint_t::serialize(std::span<uint8_t> output) const {
    uint32_t curr_value = value;

    uint8_t encoded;
    uint8_t i = 0;
    do {
        encoded = curr_value & 0x7F;
        curr_value = curr_value >> 7;
        if(curr_value > 0) {
            encoded |= 0x80;
        }
        if(i == output.size()) {
            return i;
        }
        output[i] = encoded;
        i++;
    } while(curr_value > 0);
    return i;
}

mqtt::varint_t::operator int() const {
    return value;
}