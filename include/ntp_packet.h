#pragma once

#include <stdint.h>
#include <span>

#define NTP_MESSAGE_LEN 48

union ntp_short {
    uint32_t data;
    struct {
        uint16_t seconds;
        uint16_t fraction;
    };
};

union ntp_timestamp {
    uint64_t data;
    struct {
        uint32_t seconds;
        uint32_t fraction;
    };

    int32_t fraction_to_ms() {
        return (int32_t)((((double)ntohl(fraction)) / UINT32_MAX) * 1000);
    }
};

union ntp_packet {
    ntp_packet(uint8_t leap = 0, uint8_t version = 0x4, uint8_t mode = 0x3)
        : m_lvm(leap << 6 | (version & 0x7) << 3 | (mode & 0x7))
        , m_stratum(0)
        , m_poll(0)
        , m_precision(0)
        , m_root_delay({0})
        , m_root_dispersion({0})
        , m_reference_id(0)
        , m_ref_timestamp({0})
        , m_ori_timestamp({0})
        , m_rx_timestamp({0})
        , m_tx_timestamp({0})
    {}

    ntp_packet(std::span<uint8_t, NTP_MESSAGE_LEN> data) {
        memcpy(this->data, data.data(), NTP_MESSAGE_LEN);
    }
    uint8_t data[NTP_MESSAGE_LEN];
    struct {
        uint8_t m_lvm, m_stratum;
        int8_t m_poll, m_precision;
        ntp_short m_root_delay, m_root_dispersion;
        uint32_t m_reference_id;
        ntp_timestamp m_ref_timestamp, m_ori_timestamp, m_rx_timestamp, m_tx_timestamp;
    };

    uint8_t leap() const {
        return (m_lvm >> 6) & 0x3;
    }

    uint8_t version() const {
        return (m_lvm >> 3) & 0x7;
    }

    uint8_t mode() const {
        return m_lvm & 0x7;
    }
};
