#include "wifi_utils.h"

#include "logger.h"

static int old_status = CYW43_LINK_DOWN;

void netif_status_callback(netif* netif) {
    info1("netif status change:\n");
    info1("    Link ");
    if(netif_is_link_up(netif)) {
        info_cont1("UP\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    } else {
        info_cont1("DOWN\n");
        for(int i = 0; i < 3; i++) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(100);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(250);
        }
    }
    const ip4_addr_t *address = netif_ip4_addr(netif);
    info1("    IP Addr: ");
    if(address) {
        info_cont("%d.%d.%d.%d\n", ip4_addr1(address), ip4_addr2(address), ip4_addr3(address), ip4_addr4(address));
    } else {
        info_cont1("(null)\n");
    }
}

const char* cyw43_tcpip_link_status_name(int status)
{
    switch (status) {
    case CYW43_LINK_DOWN:
        return "link down";
    case CYW43_LINK_JOIN:
        return "joining";
    case CYW43_LINK_NOIP:
        return "no ip";
    case CYW43_LINK_UP:
        return "link up";
    case CYW43_LINK_FAIL:
        return "link fail";
    case CYW43_LINK_NONET:
        return "network fail";
    case CYW43_LINK_BADAUTH:
        return "bad auth";
    }
    return "unknown";
}

int check_network_connection(const char* ssid, const char* password) {
    int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    if(status <= 0) {
        int err = cyw43_arch_wifi_connect_async(ssid, password, CYW43_AUTH_WPA2_AES_PSK);
        if(err != 0) {
            error("failed to start wifi scan (code %d).\n", err);
        }
    }
    if(status != old_status) {
        info("Wifi status changed: '%s' -> '%s'\n", cyw43_tcpip_link_status_name(old_status), cyw43_tcpip_link_status_name(status));
        old_status = status;
    }
    return status;
}