#pragma once

#include <string>
#include <functional>
#include <span>

#include "tcp_base.h"
#include "lwip/ip_addr.h"

#include "circular_buffer.h"
#include "logger.h"

class tcp_client : public tcp_base {
public:
    tcp_client();
    ~tcp_client();
    bool init() override;
    int available() const override;
    size_t read(std::span<uint8_t> out) override;
    bool write(std::span<const uint8_t> data) override;
    void flush() override;
    bool connect(ip_addr_t addr, uint16_t port);
    bool connect(std::string addr, uint16_t port) override;
    err_t close(err_t reason) override;

    bool connected() const override;
    bool initialized() const override;
    bool secure() const override {
        return false;
    }

    void on_receive(std::function<void()> callback) override {
        user_receive_callback = callback;
    }

    void on_connected(std::function<void()> callback) override {
        user_connected_callback = callback;
    }

    void on_poll(uint8_t interval_seconds, std::function<void()> callback);

    void on_closed(std::function<void()> callback) override {
        user_closed_callback = callback;
    }

    void on_error(std::function<void(err_t)> callback) override {
        user_error_callback = callback;
    }

    void clear_pcb() {
        tcp_controlblock = nullptr;
    }

protected:
    struct tcp_pcb *tcp_controlblock;
    ip_addr_t remote_addr;
    circular_buffer<uint8_t, BUF_SIZE> buffer;
    int buffer_len;
    int sent_len;
    bool connected_, initialized_;
    uint16_t port_;
    std::function<void()> user_receive_callback, user_connected_callback, user_poll_callback, user_closed_callback;
    std::function<void(err_t)> user_error_callback;

    bool connect();

    static void dns_callback(const char* name, const ip_addr_t *addr, void* arg);
    static err_t poll_callback(void* arg, tcp_pcb* pcb);
    static err_t sent_callback(void* arg, tcp_pcb* pcb, u16_t len);
    static err_t recv_callback(void* arg, tcp_pcb* pcb, pbuf* p, err_t err);
    //static void tcp_perror(err_t err);
    static void err_callback(void* arg, err_t err);
    static err_t connected_callback(void* arg, tcp_pcb* pcb, err_t err);
};