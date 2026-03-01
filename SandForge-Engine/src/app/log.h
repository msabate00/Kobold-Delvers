#pragma once
#include <cstdarg>
#include <cstdio>

#if defined(_WIN32)
    #include <windows.h>
#endif

inline void LogImpl(const char* file, int line, const char* format, ...)
{
    char msg[4096];
    va_list ap;
    va_start(ap, format);
    std::vsnprintf(msg, sizeof(msg), format, ap);
    va_end(ap);

    std::printf("%s\n", msg);

#if defined(_WIN32)
    char dbg[4096];
    std::snprintf(dbg, sizeof(dbg), "\n%s(%d) : %s", file, line, msg);
    OutputDebugStringA(dbg);
#endif
}

// Allows: LOG("hello"); or LOG("x=%d", x);
#define LOG(...) LogImpl(__FILE__, __LINE__, __VA_ARGS__)
