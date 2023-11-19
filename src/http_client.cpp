#include "http_client.h"

#include "logger.h"
#include "tcp_client.h"
#include "tcp_tls_client.h"

http_client::http_client(std::string url, std::span<uint8_t> cert)
    : m_host("")
    , m_url(url)
    , m_port(-1)
    , m_cert(cert)
    , m_tcp(nullptr)
    , m_user_response_callback([](){})
    , m_user_closed_callback([](){})
    , m_user_error_callback([](err_t){})
    , m_timeout_alarm(0)
    , m_timeout_ms(0)
{
    trace1("http_client ctor entered\n");
    init();
    trace1("http_client ctor exited\n");
}

http_client::~http_client() {
    trace1("http_client dtor entered\n");
    if(m_timeout_alarm != 0) {
        cancel_alarm(m_timeout_alarm);
        m_timeout_alarm = 0;
    }
    if(m_tcp) {
        delete m_tcp;
    }
    debug1("~http_client\n");
    trace1("http_client dtor exited\n");
}

void http_client::url(std::string new_url) {
    trace("http_client::url entered with new_url of '%.*s'\n", new_url.size(), new_url.data());
    m_url = new_url;
    if(m_tcp) {
        m_tcp->close(ERR_CLSD);
    }
    parse_url();
    trace1("http_client::url exited\n");
}

bool http_client::parse_url() {
    debug("http_client::parse_url '%.*s'\n", m_url.size(), m_url.data());
    m_url_parser = LUrlParser::ParseURL::parseURL(m_url);
    if(!m_url_parser.isValid()) {
        error("Invalid URL: %.*s\n", m_url.size(), m_url.data());
        trace1("http_client::parse_url exited\n");
        return false;
    }

    m_host = m_url_parser.host_;
    if(m_url_parser.port_.size() > 0)
        m_url_parser.getPort(&m_port);
    debug("http_client::parse_url got host '%.*s'\n", m_host.size(), m_host.data());

    if((m_url_parser.scheme_ == "https" || m_url_parser.scheme_ == "wss")) {
        if(m_tcp && !m_tcp->secure()) {
            delete m_tcp;
            m_tcp = nullptr;
        }

        if(m_tcp == nullptr) {
            debug1("http_client::parse_url creating new tcp_tls_client\n");
            m_tcp = new tcp_tls_client(m_cert);
        }
        if(m_port == -1) {
            m_port = 443;
        }
    } else {
        if(m_tcp && m_tcp->secure()) {
            delete m_tcp;
            m_tcp = nullptr;
        }

        if(m_tcp == nullptr) {
            debug1("http_client::parse_url creating new tcp_client\n");
            m_tcp = new tcp_client();
        }
        if(m_port == -1) {
            m_port = 80;
        }
    }
    trace1("http_client::parse_url exited\n");
    return true;
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
    trace1("http_client::header entered\n");
    if(m_request_sent) {
        m_current_request.clear();
        m_request_sent = false;
    }
    m_current_request.add_header(key, value);
    trace1("http_client::header exited\n");
}

void http_client::send_request(std::string method, std::string target, std::string body) {
    trace("http_client::send_request entered with:\n    method '%.*s'\n    target '%.*s'\n    body '%.*s'\n", method.size(), method.data(), target.size(), target.data(), body.size(), body.data());
    if(m_request_sent) {
        m_current_request.clear();
        m_request_sent = false;
    } 
    m_current_request.method_ = method;
    m_current_request.target_ = target;
    m_current_request.body_ = body;
    m_current_request.ready_ = true;
    send_request();
    trace1("http_client::send_request exited\n");
}

tcp_base *http_client::release_tcp_client() {
    trace1("http_client::release_tcp_client entered\n");
    m_tcp->on_connected([](){});
    m_tcp->on_receive([](){});
    m_tcp->on_closed([](){});
    m_tcp->on_error([](err_t){});
    tcp_base *to_return = m_tcp;
    m_tcp = nullptr;
    trace1("http_client::release_tcp_client exited\n");
    return std::move(to_return);
}

bool http_client::init() {
    trace1("http_client::init entered\n");
    bool to_return = parse_url();
    if(!m_tcp) {
        error1("http_client::init failed to create new tcp_client\n");
        trace1("http_client::init exited\n");
        m_has_error = true;
        return false;
    }
    debug("http_client::init: tcp client %p created\n", m_tcp);
    trace1("http_client::init exited\n");
    return to_return;
}

void http_client::send_request() {
    trace1("http_client::send_request entered\n");
    debug("http_client::send_request (tcp = %p)\n", m_tcp);
    m_response_ready = false;
    trace1("http_client::send_request Adding headers\n");
    m_current_request.add_header("Host", m_host);
    m_current_request.add_header("User-Agent", "pico");
    if(m_current_request.body_.size() > 0) {
        m_current_request.add_header("Content-Length", std::to_string(m_current_request.body_.size()));
    }
    m_current_response.clear();
    if(m_current_response.request == nullptr) {
        m_current_response = http_response(&m_current_request);
    }
    trace1("http_client::send_request Adding callbacks\n");
    m_tcp->on_receive(std::bind(&http_client::tcp_recv_callback, this));
    m_tcp->on_closed(std::bind(&http_client::tcp_closed_callback, this));
    m_tcp->on_error(std::bind(&http_client::tcp_error_callback, this, std::placeholders::_1));

    bool init = m_tcp->initialized() || m_tcp->init();

    if(!init) {
        error1("http_client::send_request: Could not initialize tcp client\n");
        trace1("http_client::send_request exited\n");
        m_has_error = true;
        return;
    }

    if(!m_tcp->connected()) {
        trace1("http_client::send_request Connecting TCP\n");
        m_tcp->on_connected(std::bind(&http_client::tcp_connected_callback, this));
        m_tcp->connect(m_host, m_port);
    } else {
        trace1("http_client::send_request Already connected\n");
        tcp_connected_callback();
    }
    trace1("http_client::send_request exited\n");
}

int64_t http_client::timeout_callback(alarm_id_t alarm, void* user_data) {
    http_client* client = (http_client*)user_data;
    client->m_tcp->close(ERR_TIMEOUT);
    // Do not reschedule the alarm
    return 0;
}

void http_client::tcp_connected_callback() {
    trace1("http_client::tcp_connected_callback entered\n");
    std::string serialized = m_current_request.serialize();
    debug("http_client sending:\n%.*s\n", serialized.size(), serialized.data());
    m_tcp->write({(uint8_t*)serialized.data(), serialized.size()});
    m_request_sent = true;
    if(m_timeout_ms != 0) {
        m_timeout_alarm = add_alarm_in_ms(m_timeout_ms, timeout_callback, this, true);
        debug1("Adding timeout alarm\n");
    }
    trace1("http_client::tcp_connected_callback exited\n");
}

#define MAX_RECV_BYTE_OUTPUT 256

void http_client::tcp_recv_callback() {
    trace1("http_client::tcp_recv_callback entered\n");
    if(m_timeout_alarm != 0) {
        debug1("Cancelling timeout alarm\n");
        cancel_alarm(m_timeout_alarm);
        m_timeout_alarm = 0;
    }
    uint8_t data[m_tcp->available()];
    std::span<uint8_t> span = {(uint8_t*)data, (size_t)m_tcp->available()};
    m_tcp->read(span);
    #if LOG_LEVEL <= LOG_LEVEL_DEBUG
    if(m_current_response.state != http_response::parse_state::body || m_current_response.type != http_response::content_type::binary) {
        std::string_view string = {(char*)span.data(), span.size()};
        size_t max_size = string.find("\r\n\r\n");
        if(max_size == std::string_view::npos) {
            max_size = span.size();
        }
        debug("http_client recv'd:\n%.*s\n", max_size, (char*)span.data());
    } else {
        debug("http_client recv'd %d bytes\n", span.size());
        for (uint32_t i = 0; i < span.size() && i < MAX_RECV_BYTE_OUTPUT;) {
            if ((i & 0x0f) == 0 && i != 0) {
                debug_cont1("\n");
            } else if ((i & 0x07) == 0 && i != 0) {
                debug_cont1(" ");
            }
            debug_cont("%02x ", span[i++]);
        }
        debug_cont1("\n");
    }
    #endif
    m_current_response.parse(span);
    m_response_ready = m_current_response.state == http_response::parse_state::done;
    if(m_response_ready) {
        m_tcp->on_receive([](){});
        m_user_response_callback();
    }
    trace1("http_client::tcp_recv_callback exited\n");
}

void http_client::tcp_closed_callback() {
    debug1("http_client closed callback called\n");
    m_user_closed_callback();
}

void http_client::tcp_error_callback(err_t err) {
    trace1("http_client::tcp_error_callback entered\n");
    error("Got error: '%s'\n", tcp_perror(err).c_str());
    m_has_error = true;
    m_user_error_callback(err);
    trace1("http_client::tcp_error_callback exited\n");
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