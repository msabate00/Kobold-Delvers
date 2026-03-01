#pragma once

// Minimal logging utility.
// - Writes to stdout
// - Writes to a file inside a "logs" folder placed at project root
//   (next to assets/build/levels/src...). When running from build/, it will
//   automatically use ../logs.

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

#if __has_include(<filesystem>)
    #include <filesystem>
    namespace sf_fs = std::filesystem;
#endif

#if defined(_WIN32)
    #include <windows.h>
#endif

namespace sf_log {

    inline std::mutex& Mutex() {
        static std::mutex m;
        return m;
    }

    inline std::FILE*& FileHandle() {
        static std::FILE* f = nullptr;
        return f;
    }

    inline std::string& FilePath() {
        static std::string p;
        return p;
    }

    inline std::string Timestamp() {
        std::time_t t = std::time(nullptr);
        std::tm tmv{};
#if defined(_WIN32)
        localtime_s(&tmv, &t);
#else
        localtime_r(&t, &tmv);
#endif
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
            tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
            tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        return std::string(buf);
    }

    inline std::string ResolveProjectRoot() {
#if __has_include(<filesystem>)
        const sf_fs::path cwd = sf_fs::current_path();
        const auto hasMarker = [](const sf_fs::path& p) {
            return sf_fs::exists(p / "assets") || sf_fs::exists(p / "levels") || sf_fs::exists(p / "src");
        };
        if (hasMarker(cwd)) return cwd.string();
        if (hasMarker(cwd.parent_path())) return cwd.parent_path().string();
        return cwd.string();
#else
        return std::string(".");
#endif
    }

    inline void Shutdown() {
        std::lock_guard<std::mutex> lock(Mutex());
        if (FileHandle()) {
            std::fflush(FileHandle());
            std::fclose(FileHandle());
            FileHandle() = nullptr;
        }
    }

    inline bool Init(const char* filename = "engine.log") {
        std::lock_guard<std::mutex> lock(Mutex());

        if (FileHandle()) return true;

        std::string root = ResolveProjectRoot();
        std::string dir = root + "/logs";
        std::string path = dir + "/" + (filename ? filename : "engine.log");

#if __has_include(<filesystem>)
        std::error_code ec;
        sf_fs::create_directories(sf_fs::path(dir), ec);
#endif

        std::FILE* f = std::fopen(path.c_str(), "ab");
        if (!f) {
            FilePath().clear();
            return false;
        }

        FileHandle() = f;
        FilePath() = path;
        return true;
    }

    inline void WriteImpl(const char* file, int line, const char* format, va_list ap) {
        char msg[4096];
        std::vsnprintf(msg, sizeof(msg), format, ap);

        std::printf("%s\n", msg);

        {
            std::lock_guard<std::mutex> lock(Mutex());
            if (FileHandle()) {
                std::string ts = Timestamp();
                std::fprintf(FileHandle(), "%s | %s(%d) | %s\n", ts.c_str(), file, line, msg);
                std::fflush(FileHandle());
            }
        }

#if defined(_WIN32)
        char dbg[4096];
        std::snprintf(dbg, sizeof(dbg), "\n%s(%d) : %s", file, line, msg);
        OutputDebugStringA(dbg);
#endif
    }

    inline void LogImpl(const char* file, int line, const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        WriteImpl(file, line, format, ap);
        va_end(ap);
    }

} // namespace sf_log

#define LOG(...) sf_log::LogImpl(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_INIT(...) sf_log::Init(__VA_ARGS__)
#define LOG_SHUTDOWN() sf_log::Shutdown()
