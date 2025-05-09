CC := g++
CXXFLAGS := --std=c++11 -Iinclude -Wall -Wno-unused-function -O3
LDFLAGS := -lcrypto -lssl

SRCS := $(wildcard src/*.cc)
OBJS := $(patsubst src/%.cc,bin/%.o,$(SRCS))
OUT  := bin/caching-proxy

$(OUT): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

bin/%.o: src/%.cc | bin
	$(CC) $(CXXFLAGS) -c $< -o $@

bin:
	@mkdir -p bin

clean: bin
	rm bin/*

.PHONY: clean bin bin/%.o
