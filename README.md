# CPS(cache-proxy-server)

A CLI tool that starts a caching proxy server, it will forward requests to the actual server and 
cache the responses. If the same request is made again, it will return the cached response instead of 
forwarding the request to the server.

## Usage

1. Compile

```bash
make
```

2. Start cache server

```bash
# caching-proxy --port <port> --origin <forward_url> --keep-alive <seconds>
# keep-alive indicates the seconds of cache exists.
caching-proxy --port 3000 --origin https://dummyjson.com --keep-alive 300
```

3. Clear cache

```bash
caching-proxy --clear-cache
```

## TODO

- [ ] Support https server.
- [ ] Support multithread.
- [ ] Support multi-port caching
