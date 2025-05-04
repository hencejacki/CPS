#ifndef ERR_HPP
#define ERR_HPP

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

static inline void ErrorIf(bool cond, const char* file, const char* func, int line, const char* fmt, ...) {
    if (cond) {
        std::va_list args;
        va_start(args, fmt);
        fprintf(stderr, "[%s][%s:%d] ", file, func, line);
        vfprintf(stderr, fmt, args);
        printf("\n");
        va_end(args);
        exit(1);
    }
}

#define ErrIf(cond, fmt, ...) ErrorIf(cond, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#endif // ERR_HPP