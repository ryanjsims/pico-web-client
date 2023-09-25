#include "ntp_client.h"

#include "udp_client.h"
#include "logger.h"

#define NTP_PORT 123

ntp_client::ntp_client(std::string server)
    : udp(nullptr)
    , ntp_server(server)
    , ntp_resend_alarm(0)
{
    udp->on_connect([&](){
        uint8_t ntp_data[NTP_MESSAGE_LEN] = {0};
        ntp_data[0] = 0x23;

        udp->write({ntp_data, NTP_MESSAGE_LEN});
    });

    udp->on_receive([&](const ip_addr_t* addr, uint16_t port){
        ip_addr_t remote_addr = udp->remote_address();
        if(ip_addr_cmp(addr, &remote_addr) && port == NTP_PORT) {
            uint8_t ntp_data[NTP_MESSAGE_LEN] = {0};
            udp->read({ntp_data, NTP_MESSAGE_LEN});
            ntp_client::dump_bytes({ntp_data, NTP_MESSAGE_LEN});

            uint8_t mode = ntp_data[0] & 0x7;
            if(mode == 0x4) {
                uint64_t seconds_since_1900 = ntp_data[40] << 24 | ntp_data[41] << 16 | ntp_data[42] << 8 | ntp_data[43];
            }
        }
    });
}

void ntp_client::sync_time() {
    if(!udp) {
        udp = new udp_client();
        if(!udp) {
            error1("ntp_client::sync_time: Failed to create UDP client!\n");
            return;
        }
    }

    

    udp->connect(ntp_server, NTP_PORT);
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