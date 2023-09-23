#include "http_request.h"

http_request::http_request(): method_(""), target_(""), body_("") {}

http_request::http_request(std::string method, std::string target, std::string body): method_(method), target_(target), body_(body) {
    ready_ = true;
}

void http_request::add_header(std::string key, std::string value) {
    headers[key] = value;
}

std::string http_request::serialize() {
    std::string to_return = method_ + " " + target_ + " HTTP/1.1\r\n";
    for(auto iter = headers.cbegin(); iter != headers.cend(); iter++) {
        to_return += iter->first + ": " + iter->second + "\r\n";
    }
    to_return += "\r\n" + body_;
    return to_return;
}

std::string http_request::method() const {
    return method_;
}

std::string http_request::target() const {
    return target_;
}
