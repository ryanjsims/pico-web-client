#pragma once

#include <cstring>
#include <cstdint>
#include <memory>
#include <span>

class sio_packet {
public:
    sio_packet();
    ~sio_packet();

    sio_packet& operator+=(const std::span<uint8_t> &rhs);
    sio_packet& operator+=(const std::string &rhs);
    sio_packet& operator+=(const std::string_view &rhs);

    std::span<uint8_t> span() const;
    const char* c_str() const noexcept;

private:
    uint8_t* m_payload;
    size_t m_capacity, m_size;
};