#include <map>
#include <string>

class http_request {
    friend class http_client;
public:
    http_request();
    http_request(std::string method, std::string target, std::string body = "");
    void add_header(std::string key, std::string value);
    std::string serialize();
    std::string method() const;
    std::string target() const;
    void clear();

private:
    std::string method_, target_, body_;
    std::map<std::string, std::string> headers;
    bool ready_ = false;
};
