#pragma once

#include <string>
#include <functional>
#include <span>

#include <pico/stdlib.h>

#define NTP_DEFAULT_RETRY_TIME (10 * 1000)
#define NTP_DELTA 2208988800 // Seconds between 1/1/1900 and 1/1/1970
#define NTP_MESSAGE_LEN 48

class udp_client;

class ntp_client {
public:
    ntp_client(std::string server);

    void sync_time();

    static void dump_bytes(std::span<uint8_t> data);
private:
    udp_client* udp;
    std::string ntp_server;
    alarm_id_t ntp_resend_alarm;

    void send_packet();
};