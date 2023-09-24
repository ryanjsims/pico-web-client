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

    void on_receive(std::function<void(const ip_addr_t *addr, uint16_t port)> callback);
private:
    struct udp_pcb *udp_controlblock;
    ip_addr_t remote_addr;
    uint16_t port;
    circular_buffer<uint8_t> buffer{BUF_SIZE};
    int buffer_len, sent_len;
    bool initialized_, connected_;
    std::function<void(const ip_addr_t *addr, uint16_t port)> user_receive_callback;

    bool connect();
    static void dns_callback(const char* name, const ip_addr_t *addr, void* arg);
    static void recv_callback(void* arg, udp_pcb* pcb, pbuf* p, const ip_addr_t *addr, uint16_t port);
};