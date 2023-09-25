#include "ntp_client.h"

#include <hardware/rtc.h>

#include "udp_client.h"
#include "logger.h"

#define NTP_PORT 123

ntp_client::ntp_client(std::string server)
    : udp(nullptr)
    , ntp_server(server)
    , ntp_resend_alarm(0)
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
        info("ntp_client: received packet from %d.%d.%d.%d:%d\n", ip4_addr1(addr), ip4_addr2(addr), ip4_addr3(addr), ip4_addr4(addr), port);
        ip_addr_t remote_addr = udp->remote_address();
        if(ip_addr_cmp(addr, &remote_addr) && port == NTP_PORT) {
            uint8_t ntp_data[NTP_MESSAGE_LEN] = {0};
            udp->read({ntp_data, NTP_MESSAGE_LEN});
            ntp_client::dump_bytes({ntp_data, NTP_MESSAGE_LEN});

            uint8_t mode = ntp_data[0] & 0x7;
            if(mode == 0x4) {
                uint32_t seconds_since_1900 = ntp_data[40] << 24 | ntp_data[41] << 16 | ntp_data[42] << 8 | ntp_data[43];
                uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
                time_t epoch = seconds_since_1970;
                struct tm *time_tm = gmtime(&epoch);

                datetime_t datetime = {
                    .year = (int16_t)(time_tm->tm_year + 1900),
                    .month = (int8_t)(time_tm->tm_mon + 1),
                    .day = (int8_t)time_tm->tm_mday,
                    .dotw = (int8_t)time_tm->tm_wday,
                    .hour = (int8_t)time_tm->tm_hour,
                    .min = (int8_t)time_tm->tm_min,
                    .sec = (int8_t)time_tm->tm_sec,
                };
                rtc_set_datetime(&datetime);
            } else {
                error("ntp_client: Unhandled mode 0x%02x\n", mode);
            }
        }
    });
}

void ntp_client::sync_time() {
    info1("ntp_client: sync'ing time\n");

    if(!udp->connected()) {
        udp->connect(ntp_server, NTP_PORT);
    } else {
        this->send_packet();
    }
}

void ntp_client::send_packet() {
    uint8_t ntp_data[NTP_MESSAGE_LEN] = {0};
    ntp_data[0] = 0x23;
    info1("ntp_client: Sending ntp packet\n");

    udp->write({ntp_data, NTP_MESSAGE_LEN});
}

void ntp_client::dump_bytes(std::span<uint8_t> data) {
    unsigned int i = 0;

    info("dump_bytes %d", data.size());
    for (i = 0; i < data.size();) {
        if ((i & 0x0f) == 0) {
            info_cont1("\n");
        } else if ((i & 0x07) == 0) {
            info_cont1(" ");
        }
        info_cont("%02x ", data[i++]);
    }
    info_cont1("\n");

}