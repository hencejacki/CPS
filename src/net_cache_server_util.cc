#include "net_cache_server_util.hpp"

NetCacheServerUtil::NetCacheServerUtil()
    : ip_("")
    , port_(0)
    , keep_alive_seconds_(300)
    , is_ssl_(false)
    , http_req_()
    , status_(HttpReqParseStatus::kParseRequestLine) {

}

NetCacheServerUtil::~NetCacheServerUtil() {
    CacheTimer::GetInstance().Stop();
}

NetCacheServerUtil& NetCacheServerUtil::GetInstance() {
    static NetCacheServerUtil ins;
    return ins;
}

void NetCacheServerUtil::SignalHandler(int sig)
{
    fprintf(stdout, "Exiting...\n");
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

    // Cache timer init
    CacheTimer::GetInstance().Init(5, std::max(300, keep_alive_seconds_));
}

void NetCacheServerUtil::readMsg(int clisock, std::string &req)
{
    char buffer[1024] = {0};
    int bytes_read = 0;
    bytes_read = read(clisock, buffer, sizeof(buffer));
    if (bytes_read < 0) {
        return;
    }
    req.append(buffer);
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
    if (req.empty()) return;
    size_t resp_len = req.size();
    const char* p = req.c_str();
    int parsed_bytes = 0;
    const char CRLF[] = "\r\n";
    while (status_ != HttpReqParseStatus::kParseFinish) {
        const char* line_end = std::search(p + parsed_bytes, p + resp_len, CRLF, CRLF + 2);
        std::string line(p + parsed_bytes, line_end);
        switch (status_)
        {
        case HttpReqParseStatus::kParseRequestLine:
            parseRequestLine(line);
            break;
        case HttpReqParseStatus::kParseHeaderField:
            parseHeaderField(line);
            if (resp_len - parsed_bytes <= 2) {
                status_ = HttpReqParseStatus::kParseFinish;
            }
            break;
        case HttpReqParseStatus::kParseMessageBody:
            parseMessageBody(line);
            break;
        default:
            break;
        }
        parsed_bytes += (line_end + 2 - (p + parsed_bytes));
    }
    status_ = HttpReqParseStatus::kParseRequestLine;
}

void NetCacheServerUtil::parseRequestLine(const std::string &line)
{
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$", std::regex_constants::optimize);
    std::smatch match;
    if (std::regex_match(line, match, pattern)) {
        http_req_.request_method = match[1];
        http_req_.request_url    = match[2];
        http_req_.http_version   = match[3];
        status_ = HttpReqParseStatus::kParseHeaderField;
    } else {
        status_ = HttpReqParseStatus::kParseFinish;
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
        http_req_.header[match[1]] = match[2];
        http_req_.header_origin = line;
    } else {
        status_ = HttpReqParseStatus::kParseMessageBody;
    }
}

void NetCacheServerUtil::parseMessageBody(const std::string &line)
{
    http_req_.body = line;
    status_ = HttpReqParseStatus::kParseFinish;
}

void NetCacheServerUtil::constructHttpResponse(const HttpResponse &resp_origin, std::string &resp)
{
    char buffer[1024];
    // Status line
    memset(buffer, 0, sizeof(buffer));
    snprintf(
        buffer, 
        sizeof(buffer), 
        "HTTP/%s %s %s\r\n", 
        resp_origin.http_version.c_str(), 
        resp_origin.status_code.c_str(), 
        resp_origin.status_msg.c_str()
    );
    resp.append(buffer);
    // Header field
    memset(buffer, 0, sizeof(buffer));
    resp.append(resp_origin.header_origin);
    resp.append("\r\n");
    // Response body
    resp.append(resp_origin.body);
#ifdef _DEBUG
    fprintf(stderr, "%s\n", resp.c_str());
#endif // _DEBUG
}

void NetCacheServerUtil::handleConnect(int clisock)
{
    // Read from client
    std::string req = "";
    readMsg(clisock, req);
    parseHttpRequest(req);
    // Judge cache hit or miss
    std::string cache = CacheTimer::GetInstance().GetCache(http_req_.request_url);
    HttpResponse resp_origin{};
    if (cache.empty()) {
        // Cache miss
        fprintf(stdout, "Cache miss for [%s].\n", http_req_.request_url.c_str());
        fflush(stdout);
        int ret = NetClientUtil::GetInstance()
                    .Get(http_req_.request_url.c_str(), http_req_.header_origin, resp_origin);
        if (0 != ret) {
            // Get failed
            resp_origin.http_version = "1.1";
            resp_origin.status_code = "502";
            resp_origin.status_msg = "Bad Gateway";
            resp_origin.header_origin = "X-Cache: MISS\r\n";
            resp_origin.body = "";
        } else {
            extendHeader(resp_origin, "X-Cache: MISS");
            CacheTimer::GetInstance().KeepCacheAlive(http_req_.request_url, resp_origin.body);
        }
    } else {
        // Cache Hit
        fprintf(stdout, "Cache hit for [%s].\n", http_req_.request_url.c_str());
        fflush(stdout);
        resp_origin.http_version = "1.1";
        resp_origin.status_code = "200";
        resp_origin.status_msg = "OK";
        resp_origin.header_origin = "X-Cache: HIT\r\n";
        resp_origin.body = cache;
        CacheTimer::GetInstance().KeepCacheAlive(http_req_.request_url);
    }
    std::string resp;
    constructHttpResponse(resp_origin, resp);
    writeMsg(clisock, resp);
    http_req_ = {};
    close(clisock);
}

void NetCacheServerUtil::extendHeader(HttpResponse &resp, const char *extend)
{
    std::string header_origin;
    char buffer[1024];
    for (auto it = resp.header.begin(); it != resp.header.end(); ++it) {
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s: %s\r\n", it->first.c_str(), it->second.c_str());
        header_origin.append(buffer);
    }
    header_origin.append(extend);
    header_origin.append("\r\n");
    resp.header_origin = header_origin;
}

void NetCacheServerUtil::Start(const std::string& forward_origin) {
    // Init net_client
    std::regex  pattern("^([^ ]*)://([^ ]*)$", std::regex_constants::optimize);
    std::smatch match;
    if (!std::regex_match(forward_origin, match, pattern)) {
        fprintf(stderr, "Got error forward origin format.\n");
        return;
    }
    std::string protocol_type  = match[1];
    std::string forward_domain = match[2];
    uint16_t forward_domain_port = 80;
    bool forward_origin_ssl = false;
    if (0 == protocol_type.compare("https")) {
        forward_domain_port = 443;
        forward_origin_ssl = true;
    }
    NetClientUtil::GetInstance()
        .Init(forward_domain.c_str(), forward_domain_port, 3, 3, forward_origin_ssl);
    // Start cache timer
    CacheTimer::GetInstance().Start();
    // Create socket
    int sock = -1;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    ErrIf(sock == -1, "Create socket failed.");
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

    ErrIf(-1 == bind(sock, (sockaddr*)&addr, sizeof(addr)), [&](){close(sock);}, "Bind failed.");
    ErrIf(-1 == listen(sock, SOMAXCONN), [&](){close(sock);}, "Listen failed.");

    fprintf(stdout, "Start successfully.");
    fflush(stdout);

    fd_set rfds;
    struct timespec tp;
    int ret = -1;
    while (true) {
        tp.tv_sec  = 3;
        tp.tv_nsec = 0;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        ret = pselect(sock + 1, &rfds, 0, 0, &tp, &origmask_);
        if (ret == -1 && errno == EINTR) {
            // Interrupted by signal
            close(sock);
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
            handleConnect(clisock);
        }
    }
}
