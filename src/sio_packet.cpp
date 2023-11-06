#include "sio_packet.h"

#include <math.h>
#include <pico/stdlib.h>

#include "logger.h"

sio_packet::sio_packet()
    : m_capacity(256)
    , m_size(15)
{
    m_payload = (uint8_t*)malloc(m_capacity);
    memset(m_payload, ' ', m_size);
    m_payload[m_size] = 0;
}

sio_packet::~sio_packet() {
    if(m_payload) {
        free(m_payload);
        m_payload = nullptr;
    }
}

sio_packet& sio_packet::operator+=(const std::span<uint8_t> &rhs) {
    if(m_size + rhs.size() >= m_capacity) {
        uint32_t power = log2(m_size + rhs.size());
        m_capacity = 1 << (power + 1);
        m_payload = (uint8_t*)realloc(m_payload, m_capacity);
        if(m_payload == nullptr) {
            error("sio_packet: Failed to realloc payload to capacity %d!\n", m_capacity);
            panic("Out of memory");
        }
    }
    memcpy(m_payload + m_size, rhs.data(), rhs.size());
    m_size += rhs.size();
    m_payload[m_size] = 0;
    return *this;
}

sio_packet& sio_packet::operator+=(const std::string &rhs) {
    return operator+=({(uint8_t*)rhs.data(), rhs.size()});
}

sio_packet& sio_packet::operator+=(const std::string_view &rhs) {
    return operator+=({(uint8_t*)rhs.data(), rhs.size()});
}

std::span<uint8_t> sio_packet::span() const {
    return {m_payload + 15, m_size - 15};
}

const char* sio_packet::c_str() const noexcept {
    return (const char*)m_payload;
}