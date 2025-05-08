#ifndef ERR_HPP
#define ERR_HPP

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <functional>

// Base implementation
static inline void ErrorImpl(const char* file, const char* func, int line,
                           const std::function<void()>& cb,
                           const char* msg) {
    fprintf(stderr, "[%s][%s:%d] %s\n", file, func, line, msg);
    fflush(stderr);
    cb();
    std::exit(EXIT_FAILURE);
}

// 1. No callback, simple message
static inline void ErrorIf(bool cond, const char* file, const char* func, int line,
                         const char* msg) {
    if (cond) ErrorImpl(file, func, line, []{}, msg);
}

// 2. No callback, formatted message (your requested case)
template<typename... Args>
static inline void ErrorIf(bool cond, const char* file, const char* func, int line,
                         const char* fmt, Args... args) {
    if (cond) {
        char buf[256];
        snprintf(buf, sizeof(buf), fmt, args...);
        ErrorImpl(file, func, line, []{}, buf);
    }
}

// 3. With callback, simple message
static inline void ErrorIf(bool cond, const char* file, const char* func, int line,
                         const std::function<void()>& cb,
                         const char* msg) {
    if (cond) ErrorImpl(file, func, line, cb, msg);
}

// 4. With callback, formatted message
template<typename... Args>
static inline void ErrorIf(bool cond, const char* file, const char* func, int line,
                         const std::function<void()>& cb,
                         const char* fmt, Args... args) {
    if (cond) {
        char buf[256];
        snprintf(buf, sizeof(buf), fmt, args...);
        ErrorImpl(file, func, line, cb, buf);
    }
}

// Macro to select the right version
#define ErrIf(cond, ...) \
    ErrorIf((cond), __FILE__, __func__, __LINE__, __VA_ARGS__)

#endif // ERR_HPP