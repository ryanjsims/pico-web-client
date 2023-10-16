#pragma once
#include <functional>
#include <span>
#include <string>

#include "http_request.h"
#include "http_response.h"
#include "LUrlParser.h"

class tcp_base;

class http_client {
public:
    http_client(std::string url, std::span<uint8_t> cert = {});
    http_client(http_client&&) = default;
    http_client& operator=(http_client&&) = default;
    ~http_client();

    void url(std::string new_url);

    void get(std::string target, std::string body = "");
    void post(std::string target, std::string body = "");
    void put(std::string target, std::string body = "");
    void patch(std::string target, std::string body = "");
    void del(std::string target, std::string body = "");
    void head(std::string target, std::string body = "");
    void options(std::string target, std::string body = "");

    void header(std::string key, std::string value);

    void send_request(std::string method, std::string target, std::string body = "");

    void resend_request() {
        send_request();
    }

    bool has_response() const {
        return m_response_ready;
    }

    bool sent_request() const {
        return m_request_sent;
    }

    bool connected() const;
    bool has_error() const;

    void clear_error();

    http_response &response() {
        return m_current_response;
    }

    void on_response(std::function<void()> callback) {
        m_user_response_callback = callback;
    }

    tcp_base *release_tcp_client();

    LUrlParser::ParseURL get_parsed_url() const {
        return m_url_parser;
    }

private:
    tcp_base *m_tcp;
    bool m_response_ready = false, m_request_sent = false, m_has_error = false;
    http_request m_current_request;
    http_response m_current_response;
    std::string m_host, m_url;
    std::span<uint8_t> m_cert;
    int m_port;
    LUrlParser::ParseURL m_url_parser;
    std::function<void()> m_user_response_callback;

    bool init();
    void send_request();

    void tcp_connected_callback();
    void tcp_recv_callback();
    void tcp_closed_callback();
};