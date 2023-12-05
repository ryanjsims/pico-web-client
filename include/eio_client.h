#pragma once

#include <cstdint>
#include "websocket.h"

class eio_client {
public:
    enum class packet_type: uint8_t {
        open = '0',
        close,
        ping,
        pong,
        message,
        upgrade,
        noop
    };

    eio_client(ws::websocket *socket);
    eio_client(tcp_base *socket);
    ~eio_client() { 
        trace1("~eio_client\n");
        delete m_socket;
    }

    size_t read(std::span<uint8_t> data);
    // Needs 1 + 14 add'l bytes to encode message
    bool send_message(std::span<uint8_t> data);
    uint32_t packet_size() const;

    void on_open(std::function<void()> callback);
    void on_receive(std::function<void()> callback);
    void on_closed(std::function<void()> callback);
    void on_error(std::function<void(err_t)> callback);

    void read_initial_packet();
    void set_refresh_watchdog();

private:
    ws::websocket *m_socket;
    std::function<void()> m_user_receive_callback, m_user_open_callback, m_user_close_callback;
    std::function<void(err_t)> m_user_error_callback;
    std::string m_sid;
    int m_ping_interval, m_ping_timeout, m_ping_milliseconds;
    bool m_open, m_refresh_watchdog;

    void ws_recv_callback();
    void ws_poll_callback();
    void ws_close_callback();
    void ws_error_callback(err_t reason);
};