#include "net_cache_server_util.hpp"

NetCacheServerUtil::NetCacheServerUtil()
    : ip_("")
    , port_(0)
    , keep_alive_seconds_(300)
    , is_ssl_(false)
    , http_req_()
    , status_(ParseStatus::kParseRequestLine) {

}

NetCacheServerUtil::~NetCacheServerUtil() {

}

NetCacheServerUtil& NetCacheServerUtil::GetInstance() {
    static NetCacheServerUtil ins;
    return ins;
}

void NetCacheServerUtil::SignalHandler(int sig)
{
    fprintf(stdout, "Exiting...");
}

void NetCacheServerUtil::Init(int port, int keep_alive_seconds, const char *ip, bool is_ssl)
{
    ip_                 = ip;
    port_               = static_cast<uint16_t>(port);
    keep_alive_seconds_ = keep_alive_seconds;
    is_ssl_             = is_ssl;

    // Signal handle
    sa_.sa_handler = &NetCacheServerUtil::SignalHandler;
    sigemptyset(&sa_.sa_mask);
    sa_.sa_flags = 0;
    sigaction(SIGINT, &sa_, 0);

    // Block SIGINT before pselect
    sigemptyset(&sigmask_);
    sigaddset(&sigmask_, SIGINT);
    sigprocmask(SIG_BLOCK, &sigmask_, &origmask_);
}

void NetCacheServerUtil::readMsg(int clisock, std::string &req)
{
    char buffer[1024] = {0};
    int bytes_read = 0;
    while ((bytes_read = read(clisock,buffer, sizeof(buffer))) > 0) {
        req.append(buffer);
    }
#ifdef _DEBUG
    fprintf(stderr, "%s\n", req.c_str());
#endif // _DEBUG
}

void NetCacheServerUtil::writeMsg(int clisock, const std::string& resp)
{
    int bytes_write = write(clisock, resp.c_str(), resp.size());
    if (bytes_write < 0) {
        fprintf(stderr, "Write operation failed.");
    }
}

void NetCacheServerUtil::parseHttpRequest(const std::string &req)
{
}

void NetCacheServerUtil::parseRequestLine(const std::string &line)
{
    std::regex pattern("^HTTP/([^ ]*) ([^ ]*) ([^ ]*)$", std::regex_constants::optimize);
    std::smatch match;
    if (std::regex_match(line, match, pattern)) {
        
        status_ = ParseStatus::kParseHeaderField;
    } else {
        status_ = ParseStatus::kParseFinish;
#ifdef _DEBUG
        fprintf(stderr, "Failed to parse status line: [%s].\n", line.c_str());
#endif // _DEBUG
    }
}

void NetCacheServerUtil::parseHeaderField(const std::string &line)
{
    std::regex pattern("^([^ ]*): ?(.*)$", std::regex_constants::optimize);
    std::smatch match;
    if (std::regex_match(line, match, pattern)) {
        
    } else {
        status_ = ParseStatus::kParseMessageBody;
    }
}

void NetCacheServerUtil::parseMessageBody(const std::string &line)
{
    
    status_ = ParseStatus::kParseFinish;
}

void NetCacheServerUtil::Start() {
    int sock = -1;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    ErrIf(sock == -1, "Create socket failed.");
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

    ErrIf(-1 == bind(sock, (sockaddr*)&addr, sizeof(addr)), [&](){close(sock);}, "Bind failed.");
    ErrIf(-1 == listen(sock, SOMAXCONN), [&](){close(sock);}, "Listen failed.");

    fd_set rfds;
    struct timespec tp;
    int ret = -1;
    while (true) {
        tp.tv_sec  = 3;
        tp.tv_nsec = 0;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        ret = pselect(1, &rfds, 0, 0, &tp, &origmask_);
        if (ret == -1 && errno == EINTR) {
            // Interrupted by signal
            break;
        } else if (ret == -1) {
            // Error occurs
        } else if (ret == 0) {
            // Timeout
        } else {
            struct sockaddr_in cliaddr;
            socklen_t clilen;
            int clisock = ::accept(sock, (sockaddr*)&cliaddr, &clilen);
            ErrIf(-1 == clisock, [&](){close(sock);}, "Accept operation failed.");
            // Read from client
            std::string req;
            readMsg(clisock, req);
            parseHttpRequest(req);
        }
    }
}
