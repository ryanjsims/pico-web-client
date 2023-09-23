#pragma once
#include <functional>
#include <string>

#include "http_request.h"
#include "http_response.h"
#include "LUrlParser.h"

class tcp_base;

class http_client {
public:
    http_client(std::string url);
    http_client(http_client&&) = default;
    http_client& operator=(http_client&&) = default;
    ~http_client();

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
        return response_ready;
    }

    const http_response &response() {
        response_ready = false;
        return current_response;
    }

    void on_response(std::function<void()> callback) {
        user_response_callback = callback;
    }

    tcp_base *release_tcp_client();

    LUrlParser::ParseURL get_parsed_url() const {
        return URL;
    }

private:
    tcp_base *tcp;
    bool response_ready = false;
    http_request current_request;
    http_response current_response;
    std::string host_, url_;
    int port_;
    LUrlParser::ParseURL URL;
    std::function<void()> user_response_callback;

    bool init();
    void send_request();

    void tcp_connected_callback();
    void tcp_recv_callback();
    void tcp_closed_callback();
};