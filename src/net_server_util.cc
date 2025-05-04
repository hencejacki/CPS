#include "net_server_util.hpp"

NetServerUtil::NetServerUtil(): ip_(""), port_(0), keep_alive_seconds_(300), is_ssl_(false), stop_(false) {

}

NetServerUtil::~NetServerUtil() {

}

NetServerUtil& NetServerUtil::GetInstance() {
    static NetServerUtil ins;
    return ins;
}

void NetServerUtil::Init(int port, int keep_alive_seconds, const char *ip, bool is_ssl)
{
    ip_                 = ip;
    port_               = static_cast<uint16_t>(port);
    keep_alive_seconds_ = keep_alive_seconds;
    is_ssl_             = is_ssl;

    // Signal handler
    
}

void NetServerUtil::Start() {
    int sock = -1;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    ErrIf(sock == -1, "Create socket failed.");
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

    ErrIf(-1 == bind(sock, (sockaddr*)&addr, sizeof(addr)), "Bind failed.");
    ErrIf(-1 == listen(sock, SOMAXCONN), "Listen failed.");

    fd_set rfds;
    struct timespec tp;
    int ret = -1;
    while (!stop_) {
        tp.tv_sec  = 3;
        tp.tv_nsec = 0;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

    }
}