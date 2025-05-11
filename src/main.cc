#include "getopt.h"
#include "net_cache_server_util.hpp"

struct option longopts[] = {
    {"port", required_argument, 0, 0},
    {"origin", required_argument, 0, 1},
    {"keep-alive", required_argument, 0, 2},
    {"clear-cache", no_argument, 0, 3},
    {0, 0, 0, 0}};

void CheckCacheServerStarted() {
    int pipe_fd = open(NAMED_PIPE, O_RDWR);
    if (-1 != pipe_fd) {
        fprintf(stderr, "Cache proxy server has started!");
        close(pipe_fd);
        exit(EXIT_FAILURE);
    }
    close(pipe_fd);
}

int main(int argc, char *const argv[])
{
    ErrIf(
        argc < 2, 
        "Usage:\r\n%s --port <number> --origin <url> --keep-alive <seconds> or %s --clear-cache", 
        argv[0], 
        argv[0]
    );
    int c = 0;
    int longindex = -1;
    int forward_port = 3000;
    const char *forward_origin = "https://dummyjson.com";
    int keep_alive_seconds = 300;
    PipeMessage msg{
        .pid = getpid()
    };
    int pipe_fd = -1;
    while ((c = getopt_long(argc, argv, "", longopts, &longindex)) != -1)
    {
        switch (c)
        {
        case 0:
            forward_port = atoi(optarg);
            break;
        case 1:
            forward_origin = optarg;
            break;
        case 2:
            keep_alive_seconds = atoi(optarg);
            break;
        case 3:
            // Clear cache
            pipe_fd = open(NAMED_PIPE, O_WRONLY);
            if (-1 == pipe_fd) {
                fprintf(stderr, "Cache proxy server haven't started!");
                close(pipe_fd);
                exit(EXIT_FAILURE);
            }
            if (write(pipe_fd, &msg, sizeof(msg)) < 0) {
                fprintf(stderr, "Write into pipe failed\n");
                close(pipe_fd);
                exit(EXIT_FAILURE);
            }
            fprintf(stdout, "Cache has been cleared.");
            close(pipe_fd);
            exit(EXIT_SUCCESS);
            break;
        case '?':
        default:
            ErrIf(true, "Usage:\r\n%s --port <number> --origin <url> --keep-alive <seconds> or %s --clear-cache", argv[0], argv[0]);
        }
    }
    ErrIf(longindex == -1, "Usage:\r\n%s --port <number> --origin <url> --keep-alive <seconds> or %s --clear-cache", argv[0], argv[0]);
#ifdef _DEBUG
    fprintf(stdout, "forward_port: %d, forward_origin: %s, keep-alive: %d.\n", forward_port, forward_origin, keep_alive_seconds);
    fprintf(stdout, "pipe_path: %s.\n", NAMED_PIPE);
    fflush(stdout);
#endif // _DEBUG
    CheckCacheServerStarted();
    NetCacheServerUtil::GetInstance().Init(forward_port, keep_alive_seconds);
    NetCacheServerUtil::GetInstance().Start(forward_origin);
    unlink(NAMED_PIPE);
    exit(EXIT_SUCCESS);
}
