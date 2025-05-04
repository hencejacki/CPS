#ifndef NET_CLIENT_UTIL_HPP
#define NET_CLIENT_UTIL_HPP

#include <string>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "err.hpp"

enum class ParseStatus {
    kParseStatusLine,
    kParseHeaderField,
    kParseMessageBody,
    kParseFinish
};

struct HttpResponse {
    std::string http_version;
    std::string status_code;
    std::string status_msg;
    std::unordered_map<std::string, std::string> header;
    std::string body;
};

class NetClientUtil {
public:
    NetClientUtil(const NetClientUtil&) = delete;
    NetClientUtil(const NetClientUtil&&) = delete;
    NetClientUtil& operator=(const NetClientUtil&) = delete;
    NetClientUtil& operator=(const NetClientUtil&&) = delete;
    ~NetClientUtil();

    static NetClientUtil& GetInstance();

    void Init(const char* domain, uint16_t port, int timeout = 3, int max_retry = 3, bool is_ssl = false);

    /*
     * @brief Make HTTP/HTTPS get request.
     * @param endpoint URL
     * @param header request header
     * @param resp Response message
     */
    int Get(const char* endpoint, const std::string& header, std::string& resp);

private:
    NetClientUtil();

    void constructGetRequest(const char* endpoint, const std::string& header, std::string& req);

    void parseHttpResponse(const std::string& resp, std::string& content);

    void parseStatusLine(const std::string& line);

    void parseHeaderField(const std::string& line);

    void parseMessageBody(const std::string& line);

    int netRead(int sock, char* buf, size_t n, SSL* ssl);

    int netWrite(int sock, const char* buf, size_t n, SSL* ssl);

private:
    std::string domain_;
    uint16_t port_;
    int timeout_;
    int max_retry_;
    bool is_ssl_;
    SSL_CTX* ssl_ctx_;
    HttpResponse http_resp_;
    ParseStatus status_;
};

#endif // NET_CLIENT_UTIL_HPP