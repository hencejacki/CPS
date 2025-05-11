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
    virtual ~CacheTimer();

    static CacheTimer& GetInstance();

    /// @brief Init
    /// @param interval check interval(s)
    /// @param expires expire time(s)
    void Init(int interval = 5, int expires = 30);

    void Start();

    void Stop();

    std::string GetCache(const std::string& url);

    void KeepCacheAlive(const std::string& url, const std::string& content = "");

    void ClearCache();

private:
    CacheTimer();

    void checkInactiveCache();

private:
    std::map<TimePoint, std::string> time_map_;
    std::map<std::string, TMDBCache> cache_map_;
    std::chrono::seconds check_interval_;
    std::chrono::seconds expire_interval_;
    bool running_;
    std::mutex mtx_;
    std::thread t_;
};

#endif // CACHE_TIMER_HPP