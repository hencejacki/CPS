#ifndef CACHE_TIMER_HPP
#define CACHE_TIMER_HPP

#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <map>

using TimePoint = std::chrono::steady_clock::time_point;

struct TMDBCache {
    std::string dest_url;
    std::string cache_content;
    TimePoint last_active;
};

class CacheTimer {
public:
    CacheTimer(const CacheTimer&) = delete;
    CacheTimer(const CacheTimer&&) = delete;
    CacheTimer& operator=(const CacheTimer&) = delete;
    CacheTimer& operator=(const CacheTimer&&) = delete;

    static CacheTimer& GetInstance();

    void Init(int interval, int expires);

    void Start();

    void Stop();

    std::string GetCache(const std::string& url);

    void KeepCacheAlive(const std::string& url);

private:
    CacheTimer();

    void checkInactiveCache();

private:
    std::map<TimePoint, std::string> time_map_;
    std::map<std::string, TMDBCache> cache_map_;
    std::chrono::milliseconds check_interval_;
    std::chrono::milliseconds expire_interval_;
    bool running_;
    std::mutex mtx_;
    std::thread t_;
};

#endif // CACHE_TIMER_HPP