#include <map>
#include <string>
#include <span>

#ifndef HTTP_STATIC_SIZE
#define HTTP_DEFAULT_CAPACITY 2560
#else
#define HTTP_DEFAULT_CAPACITY 49152
#endif

class http_request;

class http_response {
    friend class http_client;
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
public:
    http_response(const http_request *request = nullptr);
    http_response(http_response&) = delete;
    http_response(http_response&&) = delete;
    ~http_response();

    http_response &operator=(http_response&) = delete;
    http_response &operator=(http_response&&);
    void parse(std::span<uint8_t> data);
    void parse_line(std::string_view line);
    const std::map<std::string, std::string_view> &get_headers() const;
    uint16_t status() const;
    const std::string_view &get_status_text() const;
    const std::string_view &get_protocol() const;
    const std::string_view &get_body() const;
    // Copies data from parameter into the response
    void add_data(std::span<uint8_t> data);
    void clear();

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
    const http_request *m_request = nullptr;
    bool only_parse_headers();
};
