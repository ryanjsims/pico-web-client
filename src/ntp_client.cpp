#include "ntp_client.h"

#include <hardware/rtc.h>
#include <string.h>

#include "udp_client.h"
#include "ntp_packet.h"
#include "logger.h"

#if LOG_LEVEL<=LOG_LEVEL_INFO
#include <pico/util/datetime.h>
#endif

#define NTP_PORT 123

ntp_client::ntp_client(std::string server, uint32_t retry_time)
    : udp(nullptr)
    , ntp_server(server)
    , ntp_resend_alarm(0)
    , ntp_retry_time(retry_time)
    , last_sync(0)
{
    rtc_init();

    udp = new udp_client();
    if(!udp) {
        error1("ntp_client: Failed to create UDP client!\n");
        return;
    }

    udp->on_connect([&](){
        this->send_packet();
    });

    udp->on_receive([&](const ip_addr_t* addr, uint16_t port){
        debug("ntp_client: received packet from %d.%d.%d.%d:%d\n", ip4_addr1(addr), ip4_addr2(addr), ip4_addr3(addr), ip4_addr4(addr), port);
        ip_addr_t remote_addr = udp->remote_address();
        if(ip_addr_cmp(addr, &remote_addr) && port == NTP_PORT) {
            ntp_packet packet;
            udp->read({packet.data, NTP_MESSAGE_LEN});
            ntp_client::dump_bytes({packet.data, NTP_MESSAGE_LEN});

            uint8_t mode = packet.mode();
            if(mode != 0x4) {
                error("ntp_client: Unhandled mode 0x%02x\n", mode);
                return;
            }
            cancel_alarm(ntp_resend_alarm);
            time_t epoch = ntp_client::time_t_from_ntp_timestamp(packet.m_tx_timestamp.seconds);
            struct tm *time_tm = gmtime(&epoch);

            datetime_t datetime = ntp_client::datetime_from_tm(*time_tm);
            rtc_set_datetime(&datetime);

            #if LOG_LEVEL<=LOG_LEVEL_INFO
            char buf[256];
            datetime_to_str(buf, sizeof(buf), &datetime);
            info("ntp_client: Time set to %s\n", buf);
            #endif
        }
    });
}

void ntp_client::sync_time(datetime_t *repeat) {
    info1("ntp_client: sync'ing time\n");
    if(repeat != nullptr) {
        rtc_cb_data = this;
        rtc_set_alarm(repeat, ntp_client::rtc_callback);
    }

    if(!udp->connected()) {
        udp->connect(ntp_server, NTP_PORT);
    } else {
        this->send_packet();
    }
}

void ntp_client::send_packet() {
    ntp_packet packet{};
    
    debug1("ntp_client: Sending ntp packet\n");

    ntp_resend_alarm = add_alarm_in_ms(ntp_retry_time, ntp_client::alarm_callback, this, true);
    udp->write({packet.data, NTP_MESSAGE_LEN});
}

void ntp_client::dump_bytes(std::span<uint8_t> data) {
#if LOG_LEVEL<=LOG_LEVEL_DEBUG
    unsigned int i = 0;

    debug("ntp_client::dump_bytes %d", data.size());
    for (i = 0; i < data.size();) {
        if ((i & 0x0f) == 0) {
            debug_cont1("\n");
        } else if ((i & 0x07) == 0) {
            debug_cont1(" ");
        }
        debug_cont("%02x ", data[i++]);
    }
    debug_cont1("\n");
#endif
}

int64_t reenable_rtc(alarm_id_t, void*) {
    rtc_enable_alarm();
    return 0;
}

void ntp_client::rtc_callback() {
    rtc_disable_alarm();
    add_alarm_in_ms(1500, reenable_rtc, nullptr, true);
    ntp_client* client = (ntp_client*)rtc_cb_data;
    client->sync_time();
}

int64_t ntp_client::alarm_callback(alarm_id_t alarm, void* user_data) {
    ntp_client* client = (ntp_client*)user_data;
    client->send_packet();
    return 0;
}

datetime_t ntp_client::datetime_from_tm(struct tm time_tm) {
    datetime_t datetime = {
        .year = (int16_t)(time_tm.tm_year + 1900),
        .month = (int8_t)(time_tm.tm_mon + 1),
        .day = (int8_t)time_tm.tm_mday,
        .dotw = (int8_t)time_tm.tm_wday,
        .hour = (int8_t)time_tm.tm_hour,
        .min = (int8_t)time_tm.tm_min,
        .sec = (int8_t)time_tm.tm_sec,
    };
    return datetime;
}

struct tm ntp_client::tm_from_datetime(datetime_t dt) {
    struct tm time_tm = {
        .tm_sec = dt.sec,
        .tm_min = dt.min,
        .tm_hour = dt.hour,
        .tm_mday = dt.day,
        .tm_mon = dt.month - 1,
        .tm_year = dt.year - 1900,
        .tm_wday = dt.dotw,
        .tm_isdst = -1,
    };
    return time_tm;
}

time_t ntp_client::time_t_from_ntp_timestamp(uint32_t ntp_timestamp) {
    uint32_t seconds_since_1900 = ntohl(ntp_timestamp);
    uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
    time_t epoch = seconds_since_1970;
    return epoch;
}

uint32_t ntp_client::ntp_timestamp_from_time_t(time_t epoch) {
    uint32_t seconds_since_1970 = (uint32_t)epoch;
    uint32_t seconds_since_1900 = seconds_since_1970 + NTP_DELTA;
    uint32_t ntp_timestamp = htonl(seconds_since_1900);
    return ntp_timestamp;
}

void* ntp_client::rtc_cb_data = nullptr;