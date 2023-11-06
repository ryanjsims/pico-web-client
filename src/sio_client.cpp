#include "sio_client.h"

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
        : raw_url(url)
    , engine(nullptr)
    , reconnect_time(nil_time)
{
    http = new http_client(url);
    query_string = "?EIO=4&transport=websocket";
    for(std::map<std::string, std::string>::const_iterator iter = query.cbegin(); iter != query.cend(); iter++) {
        query_string += "&" + iter->first + "=" + iter->second;
    }
    http->on_response(std::bind(&sio_client::http_response_callback, this));
}

sio_client::~sio_client() {
    for(auto iter = namespace_connections.begin(); iter != namespace_connections.end(); iter++) {
        iter->second.reset();
    }
    if(engine) {
        delete engine;
        engine = nullptr;
    }
    if(http) {
        delete http;
        http = nullptr;
    }
}

void sio_client::open() {
    if(!http) {
        error1("sio_client::open: http_client is nullptr\n");
        return;
    }

    http->header("Connection", "Upgrade");
    http->header("Upgrade", "websocket");
    http->header("Sec-WebSocket-Key", "8xtVmuvomB2taGWDXBxVMw==");
    http->header("Sec-WebSocket-Version", "13");

    http->get("/socket.io/" + query_string);
}

void sio_client::connect(std::string ns) {
    if(!engine) {
        error1("connect: Engine not initialized!\n");
        return;
    }
    sio_packet payload;
    payload += "0" + (ns != "/" ? ns + "," : "");
    engine->send_message(payload.span());
}

void sio_client::disconnect(std::string ns) {
    if(!engine) {
        error1("disconnect: Engine not initialized!\n");
        return;
    }
    debug("Client disconnecting namespace %s\n", ns.c_str());
    if(namespace_connections.find(ns) != namespace_connections.end()) {
        namespace_connections[ns]->m_sid = "";
        namespace_connections[ns]->disconnect_callback({"io client disconnect"});
        namespace_connections[ns].reset();
        namespace_connections.erase(ns);
        sio_packet packet;
        packet += "1" + (ns != "/" ? ns + "," : "");
        engine->send_message(packet.span());
    }
}

std::unique_ptr<sio_socket> &sio_client::socket(std::string ns) {
    if(namespace_connections.find(ns) == namespace_connections.end()) {
        debug("socket: Creating new socket for namespace '%s'\n", ns.c_str());
        namespace_connections[ns] = std::unique_ptr<sio_socket>(new sio_socket(engine, ns));
    }
    return namespace_connections[ns];
}

void sio_client::on_open(std::function<void()> callback) {
    user_open_callback = callback;
}

bool sio_client::ready() {
    return open_;
}

void sio_client::reconnect() {
    debug1("Reconnecting...\n");
    reconnect_time = nil_time;
    if(engine != nullptr) {
        delete engine;
        engine = nullptr;
    }
    if(http != nullptr) {
        delete http;
    }
    debug1("Creating new http_client\n");
    http = new http_client(raw_url);
    http->on_response(std::bind(&sio_client::http_response_callback, this));
    std::function<void()> old_open_callback = user_open_callback;
    on_open([&, old_open_callback](){
        for(auto iter = namespace_connections.begin(); iter != namespace_connections.end(); iter++) {
            this->connect(iter->first);
        }
        this->user_open_callback = old_open_callback;
    });
    open();
}

void sio_client::set_refresh_watchdog() {
    if(engine) {
        engine->set_refresh_watchdog();
    }
}

// Starts the sio_client main loop
void sio_client::run() {
    info1("Setting up watchdog...\n");
    watchdog_enable(8000000, true);
    debug1("Setting up alarm to extend watchdog to 30 seconds\n");
    watchdog_extender = add_alarm_in_us(7333333ull, alarm_callback, NULL, false);
    debug1("opening socket.io connection...\n");
    open();
    while(true) {
        if(!is_nil_time(reconnect_time) && time_reached(reconnect_time)) {
            alarms_fired = 0;
            watchdog_update();
            debug1("Setting up alarm to extend watchdog to 30 seconds\n");
            watchdog_extender = add_alarm_in_us(7333333ull, alarm_callback, NULL, false);
            this->reconnect();
        } else {
            sleep_ms(100);
        }
    }
}

void sio_client::http_response_callback() {
    info("Got http response: %d %s\n", http->response().status(), std::string(http->response().get_status_text()).c_str());
    
    if(http->response().status() == 101) {
        trace1("sio_client: creating engine\n");
        engine = new eio_client(http->release_tcp_client());
        delete http;
        http = nullptr;
        trace1("sio_client: engine created\n");
        if(!engine) {
            error1("Engine is nullptr\n");
            return;
        }
        engine->on_open([this](){
            open_ = true;
            if(this->watchdog_extender) {
                debug1("Cancelling watchdog extension\n");
                cancel_alarm(this->watchdog_extender);
                this->watchdog_extender = 0;
            } else {
                debug1("Watchdog extension timer id not set?\n");
            }
            user_open_callback();
        });
        trace1("sio_client: set engine open\n");
        engine->on_receive(std::bind(&sio_client::engine_recv_callback, this));
        engine->on_closed(std::bind(&sio_client::engine_closed_callback, this, std::placeholders::_1));
        trace1("sio_client: set engine recv\n");
        for(auto iter = namespace_connections.begin(); iter != namespace_connections.end(); iter++) {
            iter->second->update_engine(engine);
        }
        engine->read_initial_packet();
    }
}

void sio_client::engine_recv_callback() {
    debug1("sio_client::engine_recv_callback\n");
    std::string data;
    data.resize(engine->packet_size());
    engine->read({(uint8_t*)data.data(), data.size()});
    debug("read data: '%s'\n", data.c_str());
    std::string ns = "/";
    nlohmann::json body;
    trace1("created json\n");
    size_t tok_start, tok_end;
    if(data.size() > 1) {
        if((tok_end = data.find(",")) != std::string::npos && (tok_start = data.find("/")) != std::string::npos && tok_end < data.find("[")) {
            tok_start++;
            ns.append(data.begin() + tok_start, data.begin() + tok_end);
        }
    }
    trace1("read namespace\n");

    debug("Packet type: %c\n", data[0]);

    switch((packet_type)data[0]) {
    case packet_type::connect:{
        if((tok_start = data.find("{")) != std::string::npos) {
            tok_end = data.find("}");
            body = nlohmann::json::parse(data.substr(tok_start, (tok_end + 1) - tok_start));
        }
        if(namespace_connections.find(ns) == namespace_connections.end()) {
            debug("recv: Creating new socket for namespace '%s'\n", ns.c_str());
            namespace_connections[ns] = std::unique_ptr<sio_socket>(new sio_socket(engine, ns));
        } else {
            debug("Found existing socket for namespace '%s'\n", ns.c_str());
        }
        namespace_connections[ns]->connect_callback(body);
        break;
    }

    case packet_type::disconnect:
        debug("Server disconnecting namespace %s\n", ns.c_str());
        if(namespace_connections.find(ns) != namespace_connections.end()){
            namespace_connections[ns]->m_sid = "";
            namespace_connections[ns]->disconnect_callback({"io server disconnect"});
        }
        break;

    case packet_type::event:
        if((tok_start = data.find_first_of("[")) != std::string::npos) {
            tok_end = data.find_last_of("]");
            body = nlohmann::json::parse(data.substr(tok_start, (tok_end + 1) - tok_start));
        }
        if(namespace_connections.find(ns) != namespace_connections.end()) {
            namespace_connections[ns]->event_callback(body);
        }
        break;
    }
}

void sio_client::engine_closed_callback(err_t reason) {
    nlohmann::json disconnect_reason = nlohmann::json::array();
    if(reason == ERR_CLSD) {
        disconnect_reason.push_back("transport close");
    } else if(reason == ERR_TIMEOUT) {
        disconnect_reason.push_back("ping timeout");
    } else {
        disconnect_reason.push_back("transport error");
    }
    for(auto iter = namespace_connections.begin(); iter != namespace_connections.end(); iter++) {
        iter->second->m_sid = "";
        iter->second->disconnect_callback(disconnect_reason);
    }
    delete engine;
    engine = nullptr;
    for(auto iter = namespace_connections.begin(); iter != namespace_connections.end(); iter++) {
        iter->second->update_engine(engine);
    }

    reconnect_time = make_timeout_time_ms(1000);
    debug("Scheduled reconnect for time %lld\n", reconnect_time);
}