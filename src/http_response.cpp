#include "http_response.h"

#include <charconv>
#include "iequals.h"
#include "http_request.h"
#include "logger.h"
#include <string.h>

http_response::http_response(const http_request *request)
    : status_code(0)
    , state(parse_state::status_line)
    , request(request)
    , type(content_type::text)
    , index(0)
    , capacity(HTTP_DEFAULT_CAPACITY)
{
#ifndef HTTP_STATIC_SIZE
    data = (uint8_t*)malloc(capacity);
    if(data == nullptr) {
        error1("http_response::http_response: could not allocate buffer\n");
    }
#endif
}

void http_response::parse(std::span<uint8_t> chunk) {
    debug1("Parsing http response:\n");
    add_data(chunk);

    std::string_view data_view = {(char*)data, index};
    size_t line_start = 0, line_end = data_view.find("\r\n");
    while(line_end != std::string::npos && (state != parse_state::body || type != content_type::binary)) {
        parse_line({data_view.begin() + line_start, data_view.begin() + line_end});
        line_start = line_end + 2;
        line_end = data_view.find("\r\n", line_start);
    }
    if(line_start < data_view.size() && state != parse_state::done) {
        parse_line({data_view.begin() + line_start, data_view.end()});
    }
}

void http_response::clear() {
    status_code = 0;
    state = parse_state::status_line;
    status_text = {};
    body = {};
    protocol = {};
    index = 0;
#ifndef HTTP_STATIC_SIZE
    if(capacity > HTTP_DEFAULT_CAPACITY) {
        capacity = HTTP_DEFAULT_CAPACITY;
        data = (uint8_t*)realloc(data, capacity);
        if(data == nullptr) {
            error1("http_response::clear: failed to reallocate data\n");
            panic("http_response::clear: failed to reallocate data\n");
        }
        memset(data, 0, capacity);
    }
#endif
    headers.clear();
}

void http_response::parse_line(std::string_view line) {
    size_t token_start, token_end;
    switch(state) {
    case parse_state::status_line:
        debug1("Parsing status line\n");
        // First find the protocol, which is from the beginning to the first space
        token_end = line.find(" ");
        protocol = {line.begin(), token_end};
        debug("    Protocol: %s\n", protocol.c_str());

        // Then parse the status code, 1 character after the first space to the second space
        //   std::from_chars converts it to an integer
        token_start = token_end + 1;
        token_end = line.find(" ", token_start);
        std::from_chars(line.begin() + token_start, line.begin() + token_end, status_code);
        debug("    status code: %d\n", status_code);

        // Then the status text is the rest of the line
        token_start = token_end + 1;
        status_text = {line.begin() + token_start, line.end()};
        debug("    status text: %s\n", status_text.c_str());
        state = parse_state::headers;
        break;
    case parse_state::headers:{
        debug1("Parsing header\n");
        if(line.size() == 0) {
            debug("Empty header, transition to %s\n", only_parse_headers() ? "body" : "done");
            if(only_parse_headers()) {
                state = parse_state::done;
            } else {
                state = parse_state::body;
                body = {line.end() + 2, 0};
            }
            break;
        }
        token_end = line.find(": ");
        std::string key(line.begin(), token_end);
        std::string_view value(line.begin() + token_end + 2, line.end());
        debug("        %s: %s\n", key.c_str(), value.c_str());
        headers[key] = value;
        if(iequals(key, "Content-Length")) {
            std::from_chars(line.begin() + token_end + 2, line.end(), content_length);
        }
        if(iequals(key, "Content-Type")) {
            if(iequals(value, "application/json")) {
                type = content_type::json;
            } else if(value.starts_with("text/")) {
                type = content_type::text;
            } else {
                type = content_type::binary;
            }
        }
        break;
    }
    case parse_state::body:
        if(type != content_type::binary) {
            debug("Parsing body:\n%s\n", line.data());
        } else {
            debug("Parsing body:\n(binary length %d)\n", line.size());
        }
        body = {body.begin(), line.end()};
        if(body.size() == content_length) {
            debug1("Transition to done\n");
            state = parse_state::done;
        }
        break;
    default:
        error1("Shouldn't happen? parse_state == done\n");
        break;
    }
}

const std::map<std::string, std::string_view>& http_response::get_headers() const {
    return headers;
}

uint16_t http_response::status() const {
    return status_code;
}

std::string_view &http_response::get_status_text() {
    return status_text;
}

std::string_view &http_response::get_protocol() {
    return protocol;
}

std::string_view &http_response::get_body() {
    return body;
}

void http_response::add_data(std::span<uint8_t> data) {
    if(index + data.size() >= capacity) {
#ifdef HTTP_STATIC_SIZE
        error("http_response: Cannot add data of length %d - would exceed static capacity of %d\n", data.size(), capacity);
        return;
#else
        this->data = (uint8_t*)realloc(this->data, index + data.size() + 512);
        if(this->data == nullptr) {
            error("http_response::add_data: reallocating data to size %d failed!\n", index + data.size() + 512);
            panic("http_response::add_data: reallocating data to size %d failed!\n", index + data.size() + 512);
        }
        capacity = index + data.size() + 512;
#endif
    }
    memcpy(this->data + index, data.data(), data.size());
    index += data.size();
}

bool http_response::only_parse_headers() {
    return content_length <= 0 || request != nullptr && request->method() == "HEAD";
}