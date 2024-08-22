#include "http_response.h"

#include <charconv>
#include "iequals.h"
#include "http_request.h"
#include "logger.h"
#include <string.h>
#include <lwip/tcp.h>

http_response::http_response(http_request *request)
    : m_status_code(0)
    , m_state(parse_state::status_line)
    , m_request(request)
    , m_type(content_type::text)
    , m_index(0)
    , m_capacity(HTTP_DEFAULT_CAPACITY)
{
    trace1("http_response ctor entered\n");
#ifndef HTTP_STATIC_SIZE
    m_data = (uint8_t*)malloc(m_capacity);
    if(m_data == nullptr) {
        error1("http_response::http_response: could not allocate buffer\n");
    }
#endif
    trace1("http_response ctor exited\n");
}

http_response::~http_response() {
    trace1("http_response dtor entered\n");
#ifndef HTTP_STATIC_SIZE
    if(m_data) {
        debug1("http_response::~http_response: Freeing data\n");
        free(m_data);
    }
#endif
    trace1("http_response dtor exited\n");
}

http_response &http_response::operator=(http_response&& moved) {
    trace1("http_response move assignment operator entered\n");
#ifndef HTTP_STATIC_SIZE
    if(this->m_data) {
        free(this->m_data);
    }
    this->m_data = std::move(moved.m_data);
#endif
    this->m_request = moved.m_request;
    this->m_state = moved.m_state;
    this->m_status_code = moved.m_status_code;
    this->m_type = moved.m_type;
    this->m_index = moved.m_index;
#ifndef HTTP_STATIC_SIZE
    this->m_capacity = moved.m_capacity;
    moved.m_data = nullptr;
    moved.m_capacity = 0;
#endif
    moved.m_index = 0;
    trace1("http_response move assignment operator exited\n");
    return *this;
}

int32_t http_response::parse(std::span<uint8_t> chunk) {
    trace("http_response::parse entered with chunk of size %d\n", chunk.size());
    debug1("Parsing http response:\n");
    uint32_t start_index = m_index;
    std::string_view data_view;
    if(m_mode == mode::buffered || (m_mode == mode::streaming && m_state != parse_state::body)) {
        add_data(chunk);
        data_view = {(char*)m_data + start_index, (char*)m_data + m_index};
    } else {
        data_view = {(char*)chunk.data(), chunk.size()};
    }

    size_t line_start = 0, line_end = data_view.find("\r\n");
    uint32_t length_parsed = 0;

    while(line_end != std::string::npos && !(m_state == parse_state::body && m_type == content_type::binary)) {
        trace("http_response::parse start of while loop\n    line_start = %d\n    line_end = %d\n", line_start, line_end);
        uint32_t parsed_bytes = parse_line({data_view.begin() + line_start, data_view.begin() + line_end});
        line_start = line_end + 2;
        line_end = data_view.find("\r\n", line_start);
        length_parsed += parsed_bytes;
        trace1("http_response::parse end of while loop\n");
    }
    if(line_start < data_view.size() && m_state != parse_state::done) {
        trace("http_response::parse start of if statement\n    line_start = %d\n    line_end = %d\n", line_start, line_end);
        length_parsed += parse_line(std::string_view(data_view.begin() + line_start, data_view.end()));
        trace1("http_response::parse end of if statement\n");
    }
    trace1("http_response::parse exited\n");
    return length_parsed;
}

void http_response::clear() {
    trace1("http_response::clear entered\n");
    m_status_code = 0;
    m_state = parse_state::status_line;
    m_status_text = {};
    m_body = {};
    m_protocol = {};
    m_index = 0;
    memset(m_data, 0, m_capacity);
    m_headers.clear();
    trace1("http_response::clear exited\n");
}

int32_t http_response::parse_line(std::string_view line) {
    trace("http_response::parse_line entered with line of size %d\n", line.size());
    size_t token_start, token_end;
    switch(m_state) {
    case parse_state::status_line:
        debug1("Parsing status line\n");
        // First find the protocol, which is from the beginning to the first space
        token_end = line.find(" ");
        m_protocol = {line.begin(), token_end};
        debug("    Protocol: %s\n", std::string(m_protocol).c_str());

        // Then parse the status code, 1 character after the first space to the second space
        //   std::from_chars converts it to an integer
        token_start = token_end + 1;
        token_end = line.find(" ", token_start);
        std::from_chars(line.begin() + token_start, line.begin() + token_end, m_status_code);
        debug("    status code: %d\n", m_status_code);

        // Then the status text is the rest of the line
        token_start = token_end + 1;
        m_status_text = {line.begin() + token_start, line.end()};
        debug("    status text: %.*s\n", m_status_text.size(), m_status_text.data());
        m_state = parse_state::headers;
        break;
    case parse_state::headers:{
        debug1("Parsing header\n");
        if(line.size() == 0) {
            debug("Empty header, transition to %s\n", only_parse_headers() ? "done" : "body");
            if(only_parse_headers()) {
                m_state = parse_state::done;
            } else {
                m_state = parse_state::body;
                m_body_start = (uint32_t)line.end() - (uint32_t)m_data + 2;
                if(m_mode == mode::streaming) {
                    uint8_t* body_start_ptr = m_data + m_body_start;
                    uint32_t size = m_capacity - m_body_start - 1;
                    debug("Creating circular_view with start %p and size %d\n", body_start_ptr, size);
                    m_stream = circular_view<uint8_t>(body_start_ptr, size);
                    m_streamed_bytes = 0;
                }
            }
            break;
        }
        token_end = line.find(": ");
        std::string key(line.begin(), token_end);
        std::string_view value(line.begin() + token_end + 2, line.end());
        debug("        %.*s: %.*s\n", key.size(), key.data(), value.size(), value.data());
        m_headers[key] = value;
        if(iequals(key, "Content-Length")) {
            std::from_chars(line.begin() + token_end + 2, line.end(), m_content_length);
        }
        if(iequals(key, "Content-Type")) {
            if(iequals(value, "application/json")) {
                m_type = content_type::json;
            } else if(value.starts_with("text/")) {
                m_type = content_type::text;
            } else {
                m_type = content_type::binary;
            }
        }
        break;
    }
    case parse_state::body:
        if(m_type != content_type::binary) {
            debug("Parsing body:\n%.*s\n", line.size(), line.data());
        } else {
            debug("Parsing body:\n(binary length %d)\n", line.size());
        }
        if(m_mode == mode::buffered) {
            m_body = {(char*)m_data + m_body_start, (char*)m_data + m_index};
            debug("Body has size %d/%d\n", m_body.size(), m_content_length);
            if(m_body.size() == m_content_length) {
                debug1("Transition to done\n");
                m_state = parse_state::done;
            }
        } else {
            uint32_t bytes_written = m_stream.put({(uint8_t*)line.data(), line.size()});
            m_streamed_bytes += bytes_written;
            debug("Streamed %d/%d bytes\n", m_streamed_bytes, m_content_length);
            debug("stream has size %d\n", m_stream.size());
            if(m_streamed_bytes == m_content_length) {
                debug1("Stream transition to done\n");
                m_state = parse_state::done;
            }
            return bytes_written;
        }
        break;
    default:
        error1("Shouldn't happen? parse_state == done\n");
        break;
    }
    trace1("http_response::parse_line exited\n");
    return line.size();
}

size_t http_response::capacity_remaining() const {
    if(m_mode == mode::buffered) {
        return m_capacity - m_index;
    } else {
        return m_stream.capacity() - m_stream.size();
    }
}

size_t http_response::available() const {
    if(m_mode == mode::buffered) {
        warn1("http_response::available: Available is meant for use in streaming mode only\n");
        return 0;
    }
    if(m_state != parse_state::body && m_state != parse_state::done) {
        return 0;
    }
    return m_stream.size();
}

const std::map<std::string, std::string_view>& http_response::get_headers() const {
    return m_headers;
}

uint16_t http_response::status() const {
    return m_status_code;
}

const std::string_view &http_response::get_status_text() const {
    return m_status_text;
}

const std::string_view &http_response::get_protocol() const {
    return m_protocol;
}

const std::string_view &http_response::get_body() const {
    if(m_mode == mode::streaming) {
        warn1("http_response::get_body: Response is in streaming mode so the body cannot be retrieved as a buffer\nUse http_response::read instead\n");
    }
    return m_body;
}

int http_response::read(std::span<uint8_t> data) {
    if(m_mode == mode::buffered) {
        warn1("http_response::read: Response is in buffered mode so the body cannot be retrieved via stream\nUse http_response::get_body instead\n");
        return -1;
    }
    return m_stream.get(data);
}

int http_response::peek(std::span<uint8_t> data) {
    if(m_mode == mode::buffered) {
        warn1("http_response::read: Response is in buffered mode so the body cannot be retrieved via stream\nUse http_response::get_body instead\n");
        return -1;
    }
    int i = 0;
    for(auto it = m_stream.begin(); i < data.size() && it != m_stream.end(); i++, it++) {
        data[i] = *it;
    }
    return i;
}

void http_response::set_mode(http_response::mode new_mode) {
    m_mode = new_mode;
}

http_response::mode http_response::get_mode() const {
    return m_mode;
}

void http_response::add_data(std::span<uint8_t> data) {
    trace("http_response::add_data entered with data of size %d\n", data.size());
    if(m_mode == mode::streaming && m_state == parse_state::body) {
        warn1("http_response::add_data: Cannot add data to body in streaming mode!\n");
        return;
    }
    if(m_index + data.size() >= m_capacity) {
#ifdef HTTP_STATIC_SIZE
        error("http_response: Cannot add data of length %d - would exceed static capacity of %d\n", data.size(), m_capacity);
        return;
#else
        this->m_data = (uint8_t*)realloc(this->m_data, m_index + data.size() + 512);
        if(this->m_data == nullptr) {
            error("http_response::add_data: reallocating data to size %d failed!\n", m_index + data.size() + 512);
            // TODO: do something in response besides just panic'ing - alert the client of the failure or something
            panic("http_response::add_data: reallocating data to size %d failed!\n", m_index + data.size() + 512);
        }
        m_capacity = m_index + data.size() + 512;
#endif
    }
    memcpy(this->m_data + m_index, data.data(), data.size());
    m_index += data.size();
    trace1("http_response::add_data exited\n");
}

bool http_response::only_parse_headers() {
    return m_content_length <= 0 || (m_request != nullptr && m_request->method() == "HEAD");
}