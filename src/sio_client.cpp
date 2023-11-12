#include "sio_client.h"

#ifndef SIO_HTTP_TIMEOUT
#define SIO_HTTP_TIMEOUT 30000
#endif

volatile int alarms_fired = 0;

int64_t alarm_callback(alarm_id_t id, void* user_data) {
    debug1("timer: refreshed watchdog\n");
    watchdog_update();
    if(alarms_fired < 2) {
        alarms_fired = alarms_fired + 1;
        // Reschedule the alarm for 7.33 seconds from now 3 times (gives the sio client 30 seconds to connect before reset)
        return 7333333ll;
    }
    return 0;
}

sio_client::sio_client(std::string url, std::map<std::string, std::string> query)
        : m_raw_url(url)
    , m_engine(nullptr)
    , m_reconnect_time(nil_time)
{
    m_http = new http_client(url);
    m_query_string = "?EIO=4&transport=websocket";
    for(std::map<std::string, std::string>::const_iterator iter = query.cbegin(); iter != query.cend(); iter++) {
        m_query_string += "&" + iter->first + "=" + iter->second;
    }
    m_http->on_response(std::bind(&sio_client::http_response_callback, this));
    m_http->on_error(std::bind(&sio_client::http_error_callback, this, std::placeholders::_1));
    m_http->set_timeout(SIO_HTTP_TIMEOUT);
}

sio_client::~sio_client() {
    for(auto iter = m_namespace_connections.begin(); iter != m_namespace_connections.end(); iter++) {
        iter->second.reset();
    }
    if(m_engine) {
        delete m_engine;
        m_engine = nullptr;
    }
    if(m_http) {
        delete m_http;
        m_http = nullptr;
    }
}

void sio_client::open() {
    if(!m_http) {
        error1("sio_client::open: http_client is nullptr\n");
        return;
    }

    m_http->header("Connection", "Upgrade");
    m_http->header("Upgrade", "websocket");
    m_http->header("Sec-WebSocket-Key", "8xtVmuvomB2taGWDXBxVMw==");
    m_http->header("Sec-WebSocket-Version", "13");

    m_http->get("/socket.io/" + m_query_string);
    m_state = client_state::connecting;
}

void sio_client::connect(std::string ns) {
    if(!m_engine) {
        error1("connect: Engine not initialized!\n");
        return;
    }
    debug("sio_client::connect %s\n", ns.c_str());
    sio_packet payload;
    payload += "0" + (ns != "/" ? ns + "," : "");
    m_engine->send_message(payload.span());
}

void sio_client::disconnect(std::string ns) {
    if(!m_engine) {
        error1("disconnect: Engine not initialized!\n");
        return;
    }
    debug("Client disconnecting namespace %s\n", ns.c_str());
    if(m_namespace_connections.find(ns) != m_namespace_connections.end()) {
        m_namespace_connections[ns]->m_sid = "";
        m_namespace_connections[ns]->disconnect_callback({"io client disconnect"});
        m_namespace_connections[ns].reset();
        m_namespace_connections.erase(ns);
        sio_packet packet;
        packet += "1" + (ns != "/" ? ns + "," : "");
        m_engine->send_message(packet.span());
    }
}

std::unique_ptr<sio_socket> &sio_client::socket(std::string ns) {
    if(m_namespace_connections.find(ns) == m_namespace_connections.end()) {
        debug("socket: Creating new socket for namespace '%s'\n", ns.c_str());
        m_namespace_connections[ns] = std::unique_ptr<sio_socket>(new sio_socket(m_engine, ns));
    }
    return m_namespace_connections[ns];
}

void sio_client::on_open(std::function<void()> callback) {
    m_user_open_callback = callback;
}

bool sio_client::ready() const {
    return m_open;
}

sio_client::client_state sio_client::state() const {
    return m_state;
}

void sio_client::reconnect() {
    debug1("Reconnecting...\n");
    m_reconnect_time = nil_time;
    if(m_engine != nullptr) {
        delete m_engine;
        m_engine = nullptr;
    }
    if(m_http != nullptr) {
        delete m_http;
    }
    debug1("Creating new http_client\n");
    m_http = new http_client(m_raw_url);
    m_http->on_response(std::bind(&sio_client::http_response_callback, this));
    m_http->on_error(std::bind(&sio_client::http_error_callback, this, std::placeholders::_1));
    m_http->set_timeout(SIO_HTTP_TIMEOUT);
    std::function<void()> old_open_callback = m_user_open_callback;
    on_open([&, old_open_callback](){
        for(auto iter = m_namespace_connections.begin(); iter != m_namespace_connections.end(); iter++) {
            this->connect(iter->first);
        }
        this->m_user_open_callback = old_open_callback;
    });
    open();
}

void sio_client::set_refresh_watchdog() {
    if(m_engine) {
        m_engine->set_refresh_watchdog();
    }
}

// Starts the sio_client main loop
void sio_client::run() {
    info1("Setting up watchdog...\n");
    watchdog_enable(8000000, true);
    debug1("Setting up alarm to extend watchdog to 30 seconds\n");
    m_watchdog_extender = add_alarm_in_us(7333333ull, alarm_callback, NULL, false);
    debug1("opening socket.io connection...\n");
    open();
    while(true) {
        if(!is_nil_time(m_reconnect_time) && time_reached(m_reconnect_time)) {
            alarms_fired = 0;
            watchdog_update();
            debug1("Setting up alarm to extend watchdog to 30 seconds\n");
            m_watchdog_extender = add_alarm_in_us(7333333ull, alarm_callback, NULL, false);
            this->reconnect();
        } else {
            sleep_ms(100);
        }
    }
}

void sio_client::http_response_callback() {
    info("Got http response: %d %s\n", m_http->response().status(), std::string(m_http->response().get_status_text()).c_str());
    
    if(m_http->response().status() == 101) {
        trace1("sio_client: creating engine\n");
        m_engine = new eio_client(m_http->release_tcp_client());
        delete m_http;
        m_http = nullptr;
        trace1("sio_client: engine created\n");
        if(!m_engine) {
            error1("Engine is nullptr\n");
            m_state = client_state::error;
            return;
        }
        m_engine->on_open([this](){
            m_open = true;
            if(this->m_watchdog_extender) {
                debug1("Cancelling watchdog extension\n");
                cancel_alarm(this->m_watchdog_extender);
                this->m_watchdog_extender = 0;
            } else {
                debug1("Watchdog extension timer id not set?\n");
            }
            m_user_open_callback();
        });
        trace1("sio_client: set engine open\n");
        m_engine->on_receive(std::bind(&sio_client::engine_recv_callback, this));
        m_engine->on_closed(std::bind(&sio_client::engine_closed_callback, this));
        m_engine->on_error(std::bind(&sio_client::engine_error_callback, this, std::placeholders::_1));
        trace1("sio_client: set engine recv\n");
        for(auto iter = m_namespace_connections.begin(); iter != m_namespace_connections.end(); iter++) {
            iter->second->update_engine(m_engine);
        }
        m_state = client_state::connected;
        m_engine->read_initial_packet();
    }
}

void sio_client::http_error_callback(err_t reason) {
    error("sio_client::http_error_callback %s\n", tcp_perror(reason).c_str());
    m_state = client_state::disconnected;
}

void sio_client::engine_recv_callback() {
    debug1("sio_client::engine_recv_callback\n");
    uint8_t* data = (uint8_t*)malloc(m_engine->packet_size());
    if(data == nullptr) {
        error1("engine_recv_callback: Failed to allocate memory for packet!\n");
        return;
    }
    std::span<uint8_t> span = {data, m_engine->packet_size()};
    std::string_view strview((char*)span.data(), span.size());
    m_engine->read(span);
    debug("read data: '%*s'\n", span.size(), (const char*)span.data());
    std::string ns = "/";
    nlohmann::json body;
    trace1("created json\n");
    size_t tok_start, tok_end;
    if(span.size() > 1) {
        if((tok_end = strview.find(",")) != std::string_view::npos && (tok_start = strview.find("/")) != std::string_view::npos && tok_end < strview.find("[")) {
            tok_start++;
            ns.append(strview.begin() + tok_start, strview.begin() + tok_end);
        }
    }
    trace1("read namespace\n");

    debug("Packet type: %c\n", data[0]);

    switch((packet_type)data[0]) {
    case packet_type::connect:{
        if((tok_start = strview.find("{")) != std::string::npos) {
            tok_end = strview.find("}");
            body = nlohmann::json::parse(strview.substr(tok_start, (tok_end + 1) - tok_start));
        }
        if(m_namespace_connections.find(ns) == m_namespace_connections.end()) {
            debug("recv: Creating new socket for namespace '%s'\n", ns.c_str());
            m_namespace_connections[ns] = std::unique_ptr<sio_socket>(new sio_socket(m_engine, ns));
        } else {
            debug("Found existing socket for namespace '%s'\n", ns.c_str());
        }
        m_namespace_connections[ns]->connect_callback(body);
        break;
    }

    case packet_type::disconnect:
        debug("Server disconnecting namespace %s\n", ns.c_str());
        if(m_namespace_connections.find(ns) != m_namespace_connections.end()){
            m_namespace_connections[ns]->m_sid = "";
            m_namespace_connections[ns]->disconnect_callback({"io server disconnect"});
        }
        break;

    case packet_type::event:
        if((tok_start = strview.find_first_of("[")) != std::string::npos) {
            tok_end = strview.find_last_of("]");
            body = nlohmann::json::parse(strview.substr(tok_start, (tok_end + 1) - tok_start));
        }
        if(m_namespace_connections.find(ns) != m_namespace_connections.end()) {
            m_namespace_connections[ns]->event_callback(body);
        }
        break;
    }
    free(data);
}

void sio_client::engine_closed_callback() {
    trace1("sio_client::engine_closed_callback\n");
    nlohmann::json disconnect_reason = {"transport close"};
    disconnect_engine(disconnect_reason);
}

void sio_client::engine_error_callback(err_t reason) {
    trace("sio_client::engine_error_callback: %s\n", tcp_perror(reason).c_str());
    nlohmann::json disconnect_reason = nlohmann::json::array();
    if(reason == ERR_TIMEOUT) {
        disconnect_reason.push_back("ping timeout");
    } else {
        disconnect_reason.push_back("transport error");
    }
    disconnect_engine(disconnect_reason);
}

void sio_client::disconnect_engine(nlohmann::json disconnect_reason) {
    for(auto iter = m_namespace_connections.begin(); iter != m_namespace_connections.end(); iter++) {
        iter->second->m_sid = "";
        iter->second->disconnect_callback(disconnect_reason);
    }
    delete m_engine;
    m_engine = nullptr;
    for(auto iter = m_namespace_connections.begin(); iter != m_namespace_connections.end(); iter++) {
        iter->second->update_engine(m_engine);
    }
    m_state = client_state::disconnected;
}