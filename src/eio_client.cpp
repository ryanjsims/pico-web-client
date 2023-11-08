#include "eio_client.h"
#include <charconv>
#include <cstring>
#include "hardware/watchdog.h"
#include "nlohmann/json.hpp"

class eio_packet {
public:
    eio_packet()
        : m_capacity(256)
        , m_size(14)
    {
        m_payload = (uint8_t*)malloc(m_capacity);
        memset(m_payload, ' ', m_size);
        m_payload[m_size] = 0;
    }

    ~eio_packet() {
        if(m_payload) {
            free(m_payload);
            m_payload = nullptr;
        }
    }

    eio_packet& operator+=(const std::span<uint8_t> rhs) {
        if(m_size + rhs.size() >= m_capacity) {
            uint32_t power = log2(m_size + rhs.size());
            m_capacity = 1 << (power + 1);
            m_payload = (uint8_t*)realloc(m_payload, m_capacity);
            if(m_payload == nullptr) {
                error("eio_packet: Failed to realloc payload to capacity %d!\n", m_capacity);
                panic("Out of memory");
            }
        }
        memcpy(m_payload + m_size, rhs.data(), rhs.size());
        m_size += rhs.size();
        m_payload[m_size] = 0;
        return *this;
    }

    eio_packet& operator+=(const char rhs) {
        return operator+=({(uint8_t*)&rhs, 1});
    }

    eio_packet& operator+=(const std::string &rhs) {
        return operator+=({(uint8_t*)rhs.data(), rhs.size()});
    }

    eio_packet& operator+=(const std::string_view &rhs) {
        return operator+=({(uint8_t*)rhs.data(), rhs.size()});
    }

    std::span<uint8_t> span() {
        return {m_payload + 14, m_size - 14};
    }

    const char* c_str() const noexcept {
        return (const char*)m_payload;
    }

private:
    uint8_t* m_payload;
    size_t m_capacity, m_size;
};

eio_client::eio_client(ws::websocket *socket): socket_(socket), ping_milliseconds(0), open_(false), refresh_watchdog_(false) {
    trace1("eio_client (ctor)\n");
    socket_->on_receive(std::bind(&eio_client::ws_recv_callback, this));
    socket_->on_poll(1, std::bind(&eio_client::ws_poll_callback, this));
    socket_->on_closed(std::bind(&eio_client::ws_close_callback, this, std::placeholders::_1));
}

eio_client::eio_client(tcp_base *socket): ping_milliseconds(0), open_(false), refresh_watchdog_(false) {
    trace1("eio_client (ctor)\n");
    socket_ = new ws::websocket(socket);
    socket_->on_receive(std::bind(&eio_client::ws_recv_callback, this));
    socket_->on_poll(1, std::bind(&eio_client::ws_poll_callback, this));
    socket_->on_closed(std::bind(&eio_client::ws_close_callback, this, std::placeholders::_1));
}

size_t eio_client::read(std::span<uint8_t> data) {
    return socket_->read(data);
}

bool eio_client::send_message(std::span<uint8_t> data) {
    for(int i = -15; i < 0; i++) {
        if(data[i] != ' ') {
            error1("eio_client::send_message expects 15 extra space bytes before the beginning of the given span!\n");
            return false;
        }
    }
    data[-1] = (uint8_t)packet_type::message;
    debug("EIO send message: '%*s'\n", data.size() + 1, data.data() - 1);
    return socket_->write_text({data.data() - 1, data.size() + 1});
}

uint32_t eio_client::packet_size() const {
    return socket_->received_packet_size() - 1;
}

void eio_client::on_receive(std::function<void()> callback) {
    user_receive_callback = callback;
}

void eio_client::on_closed(std::function<void(err_t)> callback) {
    user_close_callback = callback;
}

void eio_client::on_open(std::function<void()> callback) {
    user_open_callback = callback;
}

void eio_client::read_initial_packet() {
    debug1("Engine reading initial packet...\n");
    socket_->tcp_recv_callback();
    set_refresh_watchdog();
}

void eio_client::set_refresh_watchdog() {
    refresh_watchdog_ = true;
}

void eio_client::ws_recv_callback() {
    packet_type type;
    socket_->read({(uint8_t*)&type, 1});
    switch(type) {
    case packet_type::open:{
        uint8_t* packet = (uint8_t*)malloc(packet_size());
        if(packet == nullptr) {
            error1("eio_client::ws_recv_callback: failed to allocate open packet data!\n");
            panic("Out of memory!\n");
        }
        socket_->read({packet, packet_size()});
        nlohmann::json body = nlohmann::json::parse(std::string_view((char*)packet, packet_size()));
        sid = body["sid"];
        ping_interval = body["pingInterval"];
        ping_timeout = body["pingTimeout"];
        info("EIO Open:\n    sid=%s\n    pingInterval=%d\n    pingTimeout=%d\n", sid.c_str(), ping_interval, ping_timeout);
        open_ = true;
        user_open_callback();
        free(packet);
        break;
    }

    case packet_type::close:
        debug1("EIO Close\n");
        open_ = false;
        socket_->close(ERR_CLSD);
        break;

    case packet_type::ping:{
        debug1("EIO Ping\n");
        ping_milliseconds = 0;
        eio_packet response;
        response += (char)packet_type::pong;
        socket_->write_text(response.span());
        break;
    }

    case packet_type::message:
        debug1("EIO Message\n");
        user_receive_callback();
        break;
    
    default:
        error("Unexpected packet type: %c\n", type);
        break;
    }
}

void eio_client::ws_poll_callback() {
    trace1("eio_client::ws_poll_callback\n");
    if(refresh_watchdog_) {
        watchdog_update();
        trace1("refreshed watchdog\n");
    }
    if(open_) {
        ping_milliseconds += 1000;
        if(ping_milliseconds > ping_interval + ping_timeout) {
            open_ = false;
            socket_->close(ERR_TIMEOUT);
        }
    }
}

void eio_client::ws_close_callback(err_t reason) {
    user_close_callback(reason);
}