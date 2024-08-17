#include <stdio.h>
#include <charconv>
#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <http_client.h>
#include <wifi_utils.h>
#include "logger.h"

#include <lwip/dns.h>

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


int main() {
    stdio_init_all();
    sleep_ms(1000);
    dns_init();

    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        error1("Wi-Fi init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    info("Connecting to WiFi SSID %s...\n", WIFI_SSID);

    netif_set_status_callback(netif_default, netif_status_callback);

    ip_addr_t address;
    IP4_ADDR(&address, 192, 168, 0, 6);
    info1("Setting local dns host...\n");
    err_t rc = dns_local_addhost("weewx.fluorine.local", &address);

    http_client client("https://weewx.fluorine.local", {(uint8_t*)ISRG_ROOT_X1_CERT, sizeof(ISRG_ROOT_X1_CERT)});

    client.on_response([&client](){
        const http_response& response = client.response();
        info("Got response %d %.*s\n", response.status(), response.get_status_text().size(), response.get_status_text().data());
        auto length = response.get_headers().find("Content-Length");
        if(length != response.get_headers().end()) {
            info("Response length: %.*s\n", length->second.size(), length->second.data());
        } else {
            warn1("No content length in response.\n");
        }
    });

    int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    while(true) {
        link_status = check_network_connection(WIFI_SSID, WIFI_PASSWORD);
        if(link_status == CYW43_LINK_UP && (!client.sent_request() || client.has_response())) {
            if(client.has_response() && client.response().status() == 200) {
                info1("Success!\n");
            }
            info1("Requesting test image...\n");
            client.header("Connection", "keep-alive");
            client.get("/images/weathermap_test.png");
        }
        sleep_ms(5000);
    }
}