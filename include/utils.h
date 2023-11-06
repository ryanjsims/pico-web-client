#pragma once

void netif_status_callback(netif* netif);
int check_network_connection(const char* ssid, const char* password);
const char* cyw43_tcpip_link_status_name(int status);