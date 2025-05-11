#include "cache_timer.hpp"

CacheTimer::CacheTimer() : running_(false) {

}

CacheTimer::~CacheTimer() {
    Stop();
}

CacheTimer& CacheTimer::GetInstance() {
    static CacheTimer ins;
    return ins;
}

void CacheTimer::Init(int interval, int expires)
{
    check_interval_ = std::chrono::seconds(interval);
    expire_interval_ = std::chrono::seconds(expires);
}

void CacheTimer::Start() {
    running_ = true;
    t_ = std::thread([this](){
        while (running_) {
            std::this_thread::sleep_for(check_interval_);
            checkInactiveCache();
        }
    });
}

void CacheTimer::Stop() {
    running_ = false;
    if (t_.joinable()) {
        t_.join();
    }
}

std::string CacheTimer::GetCache(const std::string &url)
{
    std::lock_guard<std::mutex> lock(mtx_);
    return cache_map_.count(url) == 0 ? "" : cache_map_.find(url)->second.cache_content;
}

void CacheTimer::KeepCacheAlive(const std::string &url, const std::string& content)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = cache_map_.find(url);
    if (it != cache_map_.end()) {
        time_map_.erase(it->second.last_active);
        it->second.last_active = std::chrono::steady_clock::now();
        time_map_[it->second.last_active] = url;
    } else {
        auto now = std::chrono::steady_clock::now();
        TMDBCache cache{
            .dest_url = url,
            .cache_content = content,
            .last_active = now
        };
        cache_map_[url] = cache;
        time_map_.emplace(now, url);
    }
}

void CacheTimer::ClearCache()
{
    std::lock_guard<std::mutex> lock(mtx_);
    time_map_.clear();
    cache_map_.clear();
}

void CacheTimer::checkInactiveCache()
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto now = std::chrono::steady_clock::now();
    auto expire_point = now - expire_interval_;

    auto it = time_map_.begin();
    while (it != time_map_.end() && it->first < expire_point) {
        auto url = it->second;
        auto cache = cache_map_.find(url);
        if (cache != cache_map_.end()) {
            cache_map_.erase(url);
        }
        it = time_map_.erase(it);
    }
}
