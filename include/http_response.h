#include <map>
#include <string>

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
    void parse(const std::string &data);
    void parse_line(std::string_view line);
    const std::map<std::string, std::string> &get_headers() const;
    uint16_t status() const;
    const std::string &get_status_text() const;
    const std::string &get_protocol() const;
    const std::string &get_body() const;
    void add_data(std::string data);
    void clear();

private:
    uint16_t status_code;
    int content_length = -1;
    std::string protocol, status_text, body, data;
    std::map<std::string, std::string> headers;
    parse_state state;
    content_type type;
    const http_request *request = nullptr;
    bool only_parse_headers();
};
