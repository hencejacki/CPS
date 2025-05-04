#include "err.hpp"
#include "getopt.h"

struct option longopts[] = {
    {"port", required_argument, 0, 0},
    {"origin", required_argument, 0, 1},
    {"keep-alive", required_argument, 0, 2},
    {"clear-cache", no_argument, 0, 3},
    {0, 0, 0, 0}};

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
    const char *forward_origin = "http://dummyjson.com";
    int keep_alive_seconds = 300;
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
            fprintf(stdout, "Cache has been cleared.");
            exit(EXIT_SUCCESS);
        case '?':
        default:
            ErrIf(true, "Usage:\r\n%s --port <number> --origin <url> --keep-alive <seconds> or %s --clear-cache", argv[0], argv[0]);
        }
    }
    ErrIf(longindex == -1, "Usage:\r\n%s --port <number> --origin <url> --keep-alive <seconds> or %s --clear-cache", argv[0], argv[0]);
#ifdef _DEBUG
    fprintf(stdout, "forward_port: %d, forward_origin: %s, keep-alive: %d.", forward_port, forward_origin, keep_alive_seconds);
#endif // _DEBUG
    exit(EXIT_SUCCESS);
}
