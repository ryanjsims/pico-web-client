#include "http_response.h"

#include <charconv>
#include "iequals.h"
#include "http_request.h"
#include "logger.h"

http_response::http_response(const http_request *request): status_code(0), state(parse_state::status_line), request(request), type(content_type::text) {}

void http_response::parse(const std::string &data) {
    debug1("Parsing http response:\n");

    size_t line_start = 0, line_end = data.find("\r\n");
    while(line_end != std::string::npos && (state != parse_state::body || type != content_type::binary)) {
        parse_line({data.begin() + line_start, data.begin() + line_end});
        line_start = line_end + 2;
        line_end = data.find("\r\n", line_start);
    }
    if(line_start < data.size() && state != parse_state::done) {
        parse_line({data.begin() + line_start, data.end()});
    }
}

void http_response::parse_line(std::string_view line) {
    size_t token_start, token_end;
    switch(state) {
    case parse_state::status_line:
        debug1("Parsing status line\n");
        // First find the protocol, which is from the beginning to the first space
        token_end = line.find(" ");
        protocol = std::string(line.begin(), token_end);
        debug("    Protocol: %s\n", protocol.c_str());

        // Then parse the status code, 1 character after the first space to the second space
        //   std::from_chars converts it to an integer
        token_start = token_end + 1;
        token_end = line.find(" ", token_start);
        std::from_chars(line.begin() + token_start, line.begin() + token_end, status_code);
        debug("    status code: %d\n", status_code);

        // Then the status text is the rest of the line
        token_start = token_end + 1;
        status_text = std::string(line.begin() + token_start, line.end());
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
            }
            break;
        }
        token_end = line.find(": ");
        std::string key(line.begin(), token_end);
        std::string value(line.begin() + token_end + 2, line.end());
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
        body += line;
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

const std::map<std::string, std::string>& http_response::get_headers() const {
    return headers;
}

uint16_t http_response::status() const {
    return status_code;
}

const std::string &http_response::get_status_text() const {
    return status_text;
}

const std::string &http_response::get_protocol() const {
    return protocol;
}

const std::string &http_response::get_body() const {
    return body;
}

void http_response::add_data(std::string data) {
    this->data += data;
}

bool http_response::only_parse_headers() {
    return content_length <= 0 || request != nullptr && request->method() == "HEAD";
}