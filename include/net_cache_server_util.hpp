#ifndef NET_SERVER_UTIL_HPP
#define NET_SERVER_UTIL_HPP

#include <string>
#include <cstdint>
#include <regex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/signal.h>
#include "err.hpp"
#include "cache_timer.hpp"
#include "net_client_util.hpp"

enum class HttpReqParseStatus {
    kParseRequestLine,
    kParseHeaderField,
    kParseMessageBody,
    kParseFinish
};

struct HttpRequest {
    std::string http_version;
    std::string request_method;
    std::string request_url;
    std::unordered_map<std::string, std::string> header;
    std::string header_origin;
    std::string body;
};

class NetCacheServerUtil {
public:
    NetCacheServerUtil(const NetCacheServerUtil&) = delete;
    NetCacheServerUtil(const NetCacheServerUtil&&) = delete;
    NetCacheServerUtil& operator=(const NetCacheServerUtil&) = delete;
    NetCacheServerUtil& operator=(const NetCacheServerUtil&&) = delete;
    virtual ~NetCacheServerUtil();

    static NetCacheServerUtil& GetInstance();

    void Init(int port, int keep_alive_seconds = 300, const char* ip = "127.0.0.1", bool is_ssl = false);

    void Start(const std::string& forward_origin);

    static void SignalHandler(int sig);

private:
    NetCacheServerUtil();

    void readMsg(int clisock, std::string& req);

    void writeMsg(int clisock, const std::string& resp);

    void parseHttpRequest(const std::string& req);

    void parseRequestLine(const std::string& line);

    void parseHeaderField(const std::string& line);

    void parseMessageBody(const std::string& line);

    void constructHttpResponse(const HttpResponse& resp_origin, std::string& resp);

    void handleConnect(int clisock);

    void extendHeader(HttpResponse& resp, const char* extend);

private:
    std::string ip_;
    uint16_t port_;
    int keep_alive_seconds_;
    bool is_ssl_;

    sigset_t sigmask_, origmask_;
    struct sigaction sa_;

    HttpRequest http_req_;
    HttpReqParseStatus status_;
};

#endif // NET_SERVER_UTIL_HPP