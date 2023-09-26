#pragma once

#include <string>
#include <functional>
#include <span>
#include <cstdint>

#include "lwip/err.h"
#include "lwip/ip_addr.h"

#include "circular_buffer.h"

#define BUF_SIZE 2048
#define POLL_TIME_S 2

class udp_client {
public:
    udp_client();
    ~udp_client();
    bool init();
    int available() const;
    size_t read(std::span<uint8_t> out);
    bool write(std::span<const uint8_t> data);
    bool connect(std::string host, uint16_t port);
    bool connect(ip_addr_t host, uint16_t port);

    bool initialized() const;
    bool connected() const;
    ip_addr_t remote_address() const;

    void on_receive(std::function<void(const ip_addr_t *, uint16_t)> callback);
    void on_connect(std::function<void()> callback);
private:
    struct udp_pcb *udp_controlblock;
    ip_addr_t remote_addr;
    uint16_t port;
    circular_buffer<uint8_t, BUF_SIZE> buffer;
    int buffer_len, sent_len;
    bool initialized_, connected_;
    std::function<void(const ip_addr_t*, uint16_t)> user_receive_callback;
    std::function<void()> user_connected_callback;

    bool connect();
    static void dns_callback(const char* name, const ip_addr_t *addr, void* arg);
    static void recv_callback(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t *addr, uint16_t port);
};