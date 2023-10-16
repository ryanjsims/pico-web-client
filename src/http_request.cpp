#include "http_request.h"

#include "logger.h"

http_request::http_request(): method_(""), target_(""), body_("") {
    trace1("http_request ctor called\n");
}

http_request::http_request(std::string method, std::string target, std::string body): method_(method), target_(target), body_(body) {
    ready_ = true;
    trace("http_request ctor called with\n    method = '%s'\n    target = '%s'\n    body = '%s'\n", method.c_str(), target.c_str(), body.c_str());
}

void http_request::add_header(std::string key, std::string value) {
    trace("http_request::add_header called with '%s' = '%s'\n", key.c_str(), value.c_str());
    headers[key] = value;
}

std::string http_request::serialize() {
    trace1("http_request::serialize entered\n");
    std::string to_return = method_ + " " + target_ + " HTTP/1.1\r\n";
    for(auto iter = headers.cbegin(); iter != headers.cend(); iter++) {
        to_return += iter->first + ": " + iter->second + "\r\n";
    }
    to_return += "\r\n" + body_;
    trace1("http_request::serialize exited\n");
    return to_return;
}

std::string http_request::method() const {
    return method_;
}

std::string http_request::target() const {
    return target_;
}

void http_request::clear() {
    trace1("http_request::clear entered\n");
    method_.clear();
    target_.clear();
    body_.clear();
    for(auto it = headers.begin(); it != headers.end(); it++) {
        it->second.clear();
    }
    headers.clear();
    trace1("http_request::clear exited\n");
}