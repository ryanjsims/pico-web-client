#include "http_client.h"

#include "logger.h"
#include "tcp_client.h"
#include "tcp_tls_client.h"

http_client::http_client(std::string url): host_(""), url_(url), port_(-1) {
    init();
}

http_client::~http_client() {
    if(tcp) {
        delete tcp;
    }
    debug1("~http_client\n");
}

void http_client::get(std::string target, std::string body) {
    send_request("GET", target, body);
}

void http_client::post(std::string target, std::string body) {
    send_request("POST", target, body);
}

void http_client::put(std::string target, std::string body) {
    send_request("PUT", target, body);
}

void http_client::patch(std::string target, std::string body) {
    send_request("PATCH", target, body);
}

void http_client::del(std::string target, std::string body) {
    send_request("DELETE", target, body);
}

void http_client::head(std::string target, std::string body) {
    send_request("HEAD", target, body);
}

void http_client::options(std::string target, std::string body) {
    send_request("OPTIONS", target, body);
}

void http_client::header(std::string key, std::string value) {
    current_request.add_header(key, value);
}

void http_client::send_request(std::string method, std::string target, std::string body) {
    current_request = {method, target, body};
    send_request();
}

tcp_base *http_client::release_tcp_client() {
    tcp->on_connected([](){});
    tcp->on_receive([](){});
    tcp->on_closed([](err_t){});
    tcp_base *to_return = tcp;
    tcp = nullptr;
    return std::move(to_return);
}

bool http_client::init() {
    debug("http_client::init Parsing URL '%s'\n", url_.c_str());
    URL = LUrlParser::ParseURL::parseURL(url_);
    if(!URL.isValid()) {
        error("Invalid URL: %s\n", url_.c_str());
        return false;
    }

    host_ = URL.host_;
    if(URL.port_.size() > 0)
        URL.getPort(&port_);
    debug("http_client::init got host '%s'\n", host_.c_str());
    if(URL.scheme_ == "https" || URL.scheme_ == "wss") {
        debug1("http_client::init creating new tcp_tls_client\n");
        tcp = new tcp_tls_client();
        if(port_ == -1) {
            port_ = 443;
        }
    } else {
        debug1("http_client::init creating new tcp_client\n");
        tcp = new tcp_client();
        if(port_ == -1) {
            port_ = 80;
        }
    }
    if(!tcp) {
        error1("http_client::init failed to create new tcp_client\n");
        return false;
    }
    return true;
}

void http_client::send_request() {
    debug("http_client::send_request (tcp = %p)\n", tcp);
    response_ready = false;
    trace1("Adding headers\n");
    current_request.add_header("Host", host_);
    current_request.add_header("User-Agent", "pico");
    if(current_request.body_.size() > 0) {
        current_request.add_header("Content-Length", std::to_string(current_request.body_.size()));
    }
    current_response = http_response{&current_request};
    trace1("Adding callbacks\n");
    tcp->on_receive(std::bind(&http_client::tcp_recv_callback, this));
    tcp->on_closed(std::bind(&http_client::tcp_closed_callback, this));

    if(!tcp->initialized()) {
        trace1("Initializing TCP\n");
        tcp->init();
    }

    if(!tcp->connected()) {
        trace1("Connecting TCP\n");
        tcp->on_connected(std::bind(&http_client::tcp_connected_callback, this));
        tcp->connect(host_, port_);
    } else {
        trace1("Already connected\n");
        tcp_connected_callback();
    }
}

void http_client::tcp_connected_callback() {
    std::string serialized = current_request.serialize();
    debug("http_client sending:\n%s\n", serialized.c_str());
    tcp->write({(uint8_t*)serialized.c_str(), serialized.size()});
}

void http_client::tcp_recv_callback() {
    std::string data;
    data.resize(tcp->available());
    std::span<uint8_t> span = {(uint8_t*)data.data(), data.size()};
    tcp->read(span);
    debug("http_client recv'd:\n%s\n", data.c_str());
    current_response.parse(data);
    response_ready = current_response.state == http_response::parse_state::done;
    if(response_ready) {
        tcp->on_receive([](){});
        user_response_callback();
    }
}

void http_client::tcp_closed_callback() {
    debug1("http_client closed callback\n");
}