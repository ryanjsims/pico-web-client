#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <functional>

#include "eio_client.h"
#include "sio_packet.h"

class sio_client;
class sio_socket {
    friend class sio_client;
public:
    void on(std::string event, std::function<void(nlohmann::json)> handler);
    void once(std::string event, std::function<void(nlohmann::json)> handler);
    bool emit(std::string event, nlohmann::json array = nlohmann::json::array());
    bool connected() const;

    void update_engine(eio_client *engine_ref);

private:
    sio_socket() = default;
    sio_socket(eio_client *engine_ref, std::string ns);
    eio_client *m_engine;
    std::string m_namespace, m_sid;
    std::map<std::string, std::function<void(nlohmann::json)>> event_handlers;

    void connect_callback(nlohmann::json body);
    void disconnect_callback(nlohmann::json body = nlohmann::json::array());
    void event_callback(nlohmann::json array);
};