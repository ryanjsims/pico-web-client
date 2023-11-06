#pragma once

#include "sio_socket.h"
#include "eio_client.h"
#include "http_client.h"

#include "nlohmann/json.hpp"

#include <pico/stdlib.h>
#include <hardware/watchdog.h>

#include <map>
#include <memory>
#include <functional>

extern volatile int alarms_fired;

int64_t alarm_callback(alarm_id_t id, void* user_data);

class sio_client {
public:
    enum class packet_type: uint8_t {
        connect = '0',
        disconnect,
        event,
        ack,
        connect_error,
        binary_event,
        binary_ack
    };

    sio_client(std::string url, std::map<std::string, std::string> query);
    ~sio_client();

    void open();
    void on_open(std::function<void()> callback);

    void connect(std::string ns = "/");
    void disconnect(std::string ns = "/");
    std::unique_ptr<sio_socket> &socket(std::string ns = "/");
    void reconnect();

    bool ready();

    void set_refresh_watchdog();

    // Starts the sio_client main loop
    void run();

private:
    eio_client *engine;
    http_client *http;
    std::map<std::string, std::unique_ptr<sio_socket>> namespace_connections;
    std::function<void()> user_open_callback;
    std::function<void(err_t)> user_close_callback;
    std::string raw_url, query_string;
    bool open_ = false;
    absolute_time_t reconnect_time;
    alarm_id_t watchdog_extender = 0;

    void http_response_callback();
    void engine_recv_callback();
    void engine_closed_callback(err_t reason);
};