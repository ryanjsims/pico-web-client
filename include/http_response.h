#include <map>
#include <string>
#include <span>

#include <circular_buffer.h>
#include <lwip/tcp.h>

#ifndef HTTP_STATIC_SIZE
#define HTTP_DEFAULT_CAPACITY TCP_WND + 1024
#else
#define HTTP_DEFAULT_CAPACITY TCP_WND + 1024
#endif

class http_request;

class http_response {
    friend class http_client;
public:
    enum class parse_state {
        status_line,
        headers,
        body,
        done
    };
    enum class content_type {
        text,
        json,
        binary
    };
    enum class mode {
        buffered,
        streaming
    };
    http_response(http_request *request = nullptr);
    http_response(http_response&) = delete;
    http_response(http_response&&) = delete;
    ~http_response();

    http_response &operator=(http_response&) = delete;
    http_response &operator=(http_response&&);
    int32_t parse(std::span<uint8_t> data);
    int32_t parse_line(std::string_view line);
    const std::map<std::string, std::string_view> &get_headers() const;
    uint16_t status() const;
    const std::string_view &get_status_text() const;
    const std::string_view &get_protocol() const;
    const std::string_view &get_body() const;
    // Copies data from parameter into the response
    void add_data(std::span<uint8_t> data);
    void clear();

    void set_mode(mode new_mode);
    mode get_mode() const;
    int read(std::span<uint8_t> data);
    int peek(std::span<uint8_t> data);
    size_t capacity_remaining() const;
    size_t available() const;
    parse_state get_parse_state() const {
        return m_state;
    }

private:
    uint16_t m_status_code;
    int m_content_length = -1, m_body_start = 0;
    std::string_view m_protocol, m_status_text, m_body;
#ifndef HTTP_STATIC_SIZE
    uint8_t* m_data;
#else
    uint8_t m_data[HTTP_DEFAULT_CAPACITY];
#endif
    uint32_t m_index, m_capacity;
    std::map<std::string, std::string_view> m_headers;
    parse_state m_state;
    content_type m_type;
    mode m_mode;
    circular_view<uint8_t> m_stream;
    uint32_t m_streamed_bytes;
    http_request *m_request = nullptr;
    bool only_parse_headers();
};
