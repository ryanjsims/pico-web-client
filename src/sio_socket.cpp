#include "sio_socket.h"

sio_socket::sio_socket(eio_client *engine_ref, std::string ns)
    : m_namespace(ns)
    , m_engine(engine_ref)
{}

void sio_socket::on(std::string event, std::function<void(nlohmann::json)> handler) {
    event_handlers[event] = handler;
}

void sio_socket::once(std::string event, std::function<void(nlohmann::json)> handler) {
    event_handlers[event] = [&](nlohmann::json body) {
        handler(body);
        this->event_handlers.erase(event);
    };
}

bool sio_socket::emit(std::string event, nlohmann::json array) {
    sio_packet packet; // Add 15 bytes at the beginning to allow underlying protocols room to write data
    packet += "2" + (m_namespace != "/" ? m_namespace + "," : "");
    if(!array.is_array()) {
        array = {event, array};
    } else {
        array.insert(array.begin(), event);
    }
    packet += array.dump();
    debug("emit:\n\tNamespace '%s'\n\tpacket: '%s'\n", m_namespace.c_str(), packet.c_str());
    if(m_engine) {
        return m_engine->send_message(packet.span());
    }
    return false;
}

bool sio_socket::connected() const {
    return m_sid != "";
}

void sio_socket::update_engine(eio_client *engine_ref) {
    m_engine = engine_ref;
}

void sio_socket::connect_callback(nlohmann::json body) {
    debug("sio_socket::connect_callback\n%s\n", body.dump(4).c_str());
    if(body.contains("sid")){
        debug1("setting sid...\n");
        m_sid = body["sid"];
    }

    if(event_handlers.find("connect") != event_handlers.end()){
        event_handlers["connect"](body);
    }
}

void sio_socket::disconnect_callback(nlohmann::json body) {
    debug("sio_socket::disconnect_callback\n%s\n", body.dump(4).c_str());

    if(event_handlers.find("disconnect") != event_handlers.end()){
        event_handlers["disconnect"](body);
    }
}

void sio_socket::event_callback(nlohmann::json array) {
    if(array.size() == 0) {
        error1("Array too small!\n");
        return;
    }
    std::string event = array[0];
    array.erase(0);
    debug("sio_socket::event_callback for event '%s'\n", event.c_str());
    if(event_handlers.find(event) != event_handlers.end()) {
        event_handlers[event](array);
    }
}