#pragma once

#include <string>
#include <functional>
#include <span>

#include <pico/stdlib.h>

#define NTP_DEFAULT_RETRY_TIME (10 * 1000)
#define NTP_DELTA 2208988800 // Seconds between 1/1/1900 and 1/1/1970

class udp_client;

enum class ntp_state {
    NOT_SYNCED,
    SYNCING,
    SYNCED,
};

class ntp_client {
public:
    ntp_client(std::string server, uint32_t retry_time = NTP_DEFAULT_RETRY_TIME);

    void sync_time(datetime_t *repeat = nullptr);
    ntp_state state() const;

    static void dump_bytes(std::span<uint8_t> data);
    static datetime_t datetime_from_tm(struct tm);
    static struct tm tm_from_datetime(datetime_t);
    static struct tm localtime(datetime_t);
    static struct tm localtime(time_t utc);
    static time_t time_t_from_ntp_timestamp(uint32_t);
    static uint32_t ntp_timestamp_from_time_t(time_t);
private:
    udp_client* udp;
    ntp_state m_state;
    std::string ntp_server;
    alarm_id_t ntp_resend_alarm;
    uint32_t ntp_retry_time, last_sync, sent_ms, recv_ms;

    void send_packet();
    static void* rtc_cb_data;
    static void rtc_callback();
    static int64_t alarm_callback(alarm_id_t, void*);
};