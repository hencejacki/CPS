#ifndef NET_SERVER_UTIL_HPP
#define NET_SERVER_UTIL_HPP

#include <string>
#include <cstdint>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/signal.h>
#include "err.hpp"

class NetServerUtil {
public:
    NetServerUtil(const NetServerUtil&) = delete;
    NetServerUtil(const NetServerUtil&&) = delete;
    NetServerUtil& operator=(const NetServerUtil&) = delete;
    NetServerUtil& operator=(const NetServerUtil&&) = delete;
    virtual ~NetServerUtil();

    static NetServerUtil& GetInstance();

    void Init(int port, int keep_alive_seconds = 300, const char* ip = "127.0.0.1", bool is_ssl = false);

    void Start();

private:
    NetServerUtil();

private:
    std::string ip_;
    uint16_t port_;
    int keep_alive_seconds_;
    bool is_ssl_;
    volatile bool stop_;

    sigset_t sigmask_, origmask_;
    struct sigaction sa_;
};

#endif // NET_SERVER_UTIL_HPP