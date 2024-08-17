#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <mqtt_client.h>

#include <cctype>
#include <lwip/dns.h>

#include "logger.h"

const char ISRG_ROOT_X1_CERT[] = "-----BEGIN CERTIFICATE-----\n\
MIIFYDCCBEigAwIBAgIQQAF3ITfU6UK47naqPGQKtzANBgkqhkiG9w0BAQsFADA/\n\
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n\
DkRTVCBSb290IENBIFgzMB4XDTIxMDEyMDE5MTQwM1oXDTI0MDkzMDE4MTQwM1ow\n\
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n\
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwggIiMA0GCSqGSIb3DQEB\n\
AQUAA4ICDwAwggIKAoICAQCt6CRz9BQ385ueK1coHIe+3LffOJCMbjzmV6B493XC\n\
ov71am72AE8o295ohmxEk7axY/0UEmu/H9LqMZshftEzPLpI9d1537O4/xLxIZpL\n\
wYqGcWlKZmZsj348cL+tKSIG8+TA5oCu4kuPt5l+lAOf00eXfJlII1PoOK5PCm+D\n\
LtFJV4yAdLbaL9A4jXsDcCEbdfIwPPqPrt3aY6vrFk/CjhFLfs8L6P+1dy70sntK\n\
4EwSJQxwjQMpoOFTJOwT2e4ZvxCzSow/iaNhUd6shweU9GNx7C7ib1uYgeGJXDR5\n\
bHbvO5BieebbpJovJsXQEOEO3tkQjhb7t/eo98flAgeYjzYIlefiN5YNNnWe+w5y\n\
sR2bvAP5SQXYgd0FtCrWQemsAXaVCg/Y39W9Eh81LygXbNKYwagJZHduRze6zqxZ\n\
Xmidf3LWicUGQSk+WT7dJvUkyRGnWqNMQB9GoZm1pzpRboY7nn1ypxIFeFntPlF4\n\
FQsDj43QLwWyPntKHEtzBRL8xurgUBN8Q5N0s8p0544fAQjQMNRbcTa0B7rBMDBc\n\
SLeCO5imfWCKoqMpgsy6vYMEG6KDA0Gh1gXxG8K28Kh8hjtGqEgqiNx2mna/H2ql\n\
PRmP6zjzZN7IKw0KKP/32+IVQtQi0Cdd4Xn+GOdwiK1O5tmLOsbdJ1Fu/7xk9TND\n\
TwIDAQABo4IBRjCCAUIwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYw\n\
SwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5pZGVudHJ1\n\
c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTEp7Gkeyxx\n\
+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEB\n\
ATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQu\n\
b3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0LmNvbS9E\n\
U1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFHm0WeZ7tuXkAXOACIjIGlj26Ztu\n\
MA0GCSqGSIb3DQEBCwUAA4IBAQAKcwBslm7/DlLQrt2M51oGrS+o44+/yQoDFVDC\n\
5WxCu2+b9LRPwkSICHXM6webFGJueN7sJ7o5XPWioW5WlHAQU7G75K/QosMrAdSW\n\
9MUgNTP52GE24HGNtLi1qoJFlcDyqSMo59ahy2cI2qBDLKobkx/J3vWraV0T9VuG\n\
WCLKTVXkcGdtwlfFRjlBz4pYg1htmf5X6DYO8A4jqv2Il9DjXA6USbW1FzXSLr9O\n\
he8Y4IWS6wY7bCkjCWDcRQJMEhg76fsO3txE+FiYruq9RUWhiF1myv4Q6W+CyBFC\n\
Dfvp7OOGAN6dEOM4+qR9sdjoSYKEBpsr6GtPAQw4dy753ec5\n\
-----END CERTIFICATE-----\n\
-----BEGIN CERTIFICATE-----\n\
MIIEfjCCAuagAwIBAgIRAMlWXu0gsAES7UysKtLsddowDQYJKoZIhvcNAQELBQAw\n\
VzEeMBwGA1UEChMVbWtjZXJ0IGRldmVsb3BtZW50IENBMRYwFAYDVQQLDA1yb290\n\
QGZsdW9yaW5lMR0wGwYDVQQDDBRta2NlcnQgcm9vdEBmbHVvcmluZTAeFw0yMzEx\n\
MDgwNzI2MzRaFw0zMzExMDgwNzI2MzRaMFcxHjAcBgNVBAoTFW1rY2VydCBkZXZl\n\
bG9wbWVudCBDQTEWMBQGA1UECwwNcm9vdEBmbHVvcmluZTEdMBsGA1UEAwwUbWtj\n\
ZXJ0IHJvb3RAZmx1b3JpbmUwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGKAoIB\n\
gQDLNe4Nt98fjefLmKMBjRwn6rIRWlYaT8tAfvJBawb3Y4ZeyfqhOnJ/Pg74Yifa\n\
LKisGZ95YVseC5kNhxChGDFYgTkt0q2sugbuRkTNe1dLr1UQ6i3E6nKQ/T928UPX\n\
S64BqXQXkyji3R/VbRoNdyDvmyxZD2uLIXuo4tv0kK1cHwvoZ+lgBPrfSiuYPa3A\n\
43FBeK9DWrVM31+XyCognz8TUUd0PQPUTuk1xOgDEijQrypPHM33jeZyRv0yPYT3\n\
T1OODx0IQXbeyfRkHmfKsJxtyddRb33U2lW3m7yGBqKQE8rhr/gAA+ymzPMhk58a\n\
MPBtmSNgWAHr16J829Qx4lBtga6NfFVcV3j8OygQxYJNSmGic2FpsEtPfrO/jgWa\n\
hrxIMjGpeKzc8DVMyxTt5wqapNLuDSh18NeiLrQwW60NCc2MPkdn7f/2/sCwb7IU\n\
iM/eSgDo8sB402l+F6l0zNizvNEaVFggU5uJbnRU944PIp1d6CVkhaA7Nv7wHMMM\n\
yCcCAwEAAaNFMEMwDgYDVR0PAQH/BAQDAgIEMBIGA1UdEwEB/wQIMAYBAf8CAQAw\n\
HQYDVR0OBBYEFLCfJAY0MzytgmfMO3sUfibKqS1PMA0GCSqGSIb3DQEBCwUAA4IB\n\
gQCOxrDpm5Lytj7n93bN3yart0IDh6hBQTJoal6nQT3LjKjCBcuuxSAhZJ6HRpZ5\n\
Xx12cGlYsXdV98kTPDGmuB9SCuNEJGENPz7Fkzgkfcz7F5vS1ccth4aY6d2lJwp7\n\
IkYgHQx4rx+UsG9SeS7xohr78qinuwKEapLDYhiq5/dD0iBAuuLo0v0s5eWw3Bj1\n\
rbWqsHQd+jjComDX4npGcZ0ngu4jt3caqFUxhh5XGrcFd26QV9wQ/XBSdhWFuh/I\n\
jGOy6cmgvUFNpTMIJ7PpAZACK4vuEJ+S5PVf011LazkyUhPzvZasOkMx49cbhNnC\n\
kSIqm5U/YuMsWNVrC1HbOZPwJR3YNmLFNlcI84JfMjhRG8Z3Sc9B9UgdnCNkBeJC\n\
sFgJh/Yqc4JSzABFvXTjXQKlvN0NK6H+nCVDJwbIxjOg6254ZIFraWjrzvnRy+/H\n\
fWKHcbjCMI/QjOTtoNj36eD195qBhD1uDS7Tqvl72dxI0Pj4nqnk7j0QJmcViLOT\n\
zAs=\n\
-----END CERTIFICATE-----";

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

int main() {
    stdio_init_all();
    dns_init();

    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        error1("Wi-Fi init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    info("Connecting to WiFi SSID %s...\n", WIFI_SSID);

    netif_set_status_callback(netif_default, netif_status_callback);

    int err = cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    int status = CYW43_LINK_UP + 1;

    while(!(netif_is_link_up(netif_default) && netif_ip4_addr(netif_default)->addr != 0)) {
        if(err || status < 0) {
            if(err) {
                error("failed to start wifi scan (code %d).\n", err);
            } else {
                error("failed to join network (code %d).\n", status);
            }
            sleep_ms(1000);
            err = cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
            status = CYW43_LINK_UP + 1;
            continue;
        } 
        int new_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        if(status != new_status) {
            debug("Wifi status change: %d -> %d\n", status, new_status);
            status = new_status;
        }
    }

    ip_addr_t address;
    IP4_ADDR(&address, 192, 168, 0, 1);
    info1("Setting dns servers...\n");
    dns_setserver(0, &address);
    IP4_ADDR(&address, 1, 1, 1, 1);
    dns_setserver(1, &address);

    mqtt::client client("mqtts://homeassistant.local", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});
    client.connect(u8"pico", u8"test");

    mqtt::subscribe_packet::options_t options;
    options.qos(2);
    client.subscribe(u8"pico/test", options, [](std::u8string_view topic, const mqtt::properties& props, std::span<uint8_t> payload){
        info("Received message for topic: %.*s\n", topic.size(), topic.data());
        dump_bytes(payload.data(), payload.size());
        return mqtt::reason_code::SUCCESS;
    });

    sleep_ms(10000);

    std::vector<uint8_t> publish_data = {
        0, 1, 2, 3, 4, 5, 6, 7, 8
    };
    client.publish(u8"pico/test/send", {}, publish_data);

    sleep_ms(30000);

    client.unsubscribe(u8"pico/test");

    sleep_ms(10000);

    client.subscribe(u8"pico/test", options, [](std::u8string_view topic, const mqtt::properties& props, std::span<uint8_t> payload){
        info("Received message for topic: %.*s\n", topic.size(), topic.data());
        dump_bytes(payload.data(), payload.size());
        return mqtt::reason_code::SUCCESS;
    });

    sleep_ms(10000);

    client.disconnect(mqtt::reason_code::NORMAL_DISCONNECT);

    sleep_ms(10000);

    client.connect(u8"pico", u8"test");

    while(!client.connected()) {
        sleep_ms(100);
    }

    client.subscribe(u8"pico/test", options, [](std::u8string_view topic, const mqtt::properties& props, std::span<uint8_t> payload){
        info("Received message for topic: %.*s\n", topic.size(), topic.data());
        dump_bytes(payload.data(), payload.size());
        return mqtt::reason_code::SUCCESS;
    });

    while(true) {
        sleep_ms(100);
    }
}