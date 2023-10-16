#include "http_client.h"

#include "logger.h"
#include "tcp_client.h"
#include "tcp_tls_client.h"

http_client::http_client(std::string url, std::span<uint8_t> cert)
    : m_host("")
    , m_url(url)
    , m_port(-1)
    , m_cert(cert)
{
    init();
}

http_client::~http_client() {
    if(m_tcp) {
        delete m_tcp;
    }
    debug1("~http_client\n");
}

void http_client::url(std::string new_url) {
    m_url = new_url;
    if(m_tcp && m_tcp->connected()) {
        m_tcp->close(ERR_OK);
    }
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
    if(m_request_sent) {
        m_current_request.clear();
        m_request_sent = false;
    }
    m_current_request.add_header(key, value);
}

void http_client::send_request(std::string method, std::string target, std::string body) {
    if(m_request_sent) {
        m_current_request.clear();
        m_request_sent = false;
    } 
    m_current_request.method_ = method;
    m_current_request.target_ = target;
    m_current_request.body_ = body;
    m_current_request.ready_ = true;
    send_request();
}

tcp_base *http_client::release_tcp_client() {
    m_tcp->on_connected([](){});
    m_tcp->on_receive([](){});
    m_tcp->on_closed([](err_t){});
    tcp_base *to_return = m_tcp;
    m_tcp = nullptr;
    return std::move(to_return);
}

bool http_client::init() {
    debug("http_client::init Parsing URL '%s'\n", url_.c_str());
    m_url_parser = LUrlParser::ParseURL::parseURL(m_url);
    if(!m_url_parser.isValid()) {
        error("Invalid URL: %s\n", m_url.c_str());
        return false;
    }

    m_host = m_url_parser.host_;
    if(m_url_parser.port_.size() > 0)
        m_url_parser.getPort(&m_port);
    debug("http_client::init got host '%s'\n", host_.c_str());
    if((m_url_parser.scheme_ == "https" || m_url_parser.scheme_ == "wss") && (m_tcp == nullptr || !m_tcp->secure())) {
        debug1("http_client::init creating new tcp_tls_client\n");
        if(m_tcp) {
            delete m_tcp;
        }
        m_tcp = new tcp_tls_client(m_cert);
        if(m_port == -1) {
            m_port = 443;
        }
    } else if(m_tcp == nullptr || m_tcp->secure()) {
        debug1("http_client::init creating new tcp_client\n");
        if(m_tcp) {
            delete m_tcp;
        }
        m_tcp = new tcp_client();
        if(m_port == -1) {
            m_port = 80;
        }
    }
    if(!m_tcp) {
        error1("http_client::init failed to create new tcp_client\n");
        m_has_error = true;
        return false;
    }
    return true;
}

void http_client::send_request() {
    debug("http_client::send_request (tcp = %p)\n", tcp);
    m_response_ready = false;
    trace1("Adding headers\n");
    m_current_request.add_header("Host", m_host);
    m_current_request.add_header("User-Agent", "pico");
    if(m_current_request.body_.size() > 0) {
        m_current_request.add_header("Content-Length", std::to_string(m_current_request.body_.size()));
    }
    m_current_response.clear();
    if(m_current_response.request == nullptr) {
        m_current_response = http_response(&m_current_request);
    }
    trace1("Adding callbacks\n");
    m_tcp->on_receive(std::bind(&http_client::tcp_recv_callback, this));
    m_tcp->on_closed(std::bind(&http_client::tcp_closed_callback, this));

    bool init = m_tcp->initialized() || m_tcp->init();

    if(!init) {
        error1("http_client::send_request: Could not initialize tcp client\n");
        m_has_error = true;
        return;
    }

    if(!m_tcp->connected()) {
        trace1("Connecting TCP\n");
        m_tcp->on_connected(std::bind(&http_client::tcp_connected_callback, this));
        m_tcp->connect(m_host, m_port);
    } else {
        trace1("Already connected\n");
        tcp_connected_callback();
    }
}

void http_client::tcp_connected_callback() {
    std::string serialized = m_current_request.serialize();
    debug("http_client sending:\n%s\n", serialized.c_str());
    m_tcp->write({(uint8_t*)serialized.c_str(), serialized.size()});
    m_request_sent = true;
}

void http_client::tcp_recv_callback() {
    uint8_t data[m_tcp->available()];
    std::span<uint8_t> span = {(uint8_t*)data, (size_t)m_tcp->available()};
    m_tcp->read(span);
    debug("http_client recv'd:\n%s\n", (char*)data);
    m_current_response.parse(span);
    m_response_ready = m_current_response.state == http_response::parse_state::done;
    if(m_response_ready) {
        m_tcp->on_receive([](){});
        m_user_response_callback();
    }
}

void http_client::tcp_closed_callback() {
    debug1("http_client closed callback\n");
}

bool http_client::connected() const {
    return m_tcp->connected();
}

bool http_client::has_error() const {
    return m_has_error;
}

void http_client::clear_error() {
    m_has_error = false;
}