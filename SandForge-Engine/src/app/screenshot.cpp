#include "app/screenshot.h"
#include "app/log.h"

#include <chrono>
#include <ctime>
#include <cstring>

#if __has_include(<filesystem>)
#include <filesystem>
namespace sf_shot_fs = std::filesystem;
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../third_party/stb/stb_image_write.h"

namespace sf_screenshot {

namespace {
    static void FlipRowsRGBA(std::vector<uint8>& pixels, int w, int h)
    {
        if (w <= 0 || h <= 1 || pixels.empty()) return;
        const size_t stride = (size_t)w * 4u;
        std::vector<uint8> row(stride);
        for (int y = 0; y < h / 2; ++y) {
            uint8* a = pixels.data() + (size_t)y * stride;
            uint8* b = pixels.data() + (size_t)(h - 1 - y) * stride;
            std::memcpy(row.data(), a, stride);
            std::memcpy(a, b, stride);
            std::memcpy(b, row.data(), stride);
        }
    }
}

std::string BuildPath(Kind kind)
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t tt = system_clock::to_time_t(now);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &tt);
#else
    localtime_r(&tt, &tmv);
#endif

    char file[128];
    const bool transparent = (kind == Kind::TransparentWorld);
    std::snprintf(file, sizeof(file),
        transparent ? "screenshot_alpha_%04d-%02d-%02d_%02d-%02d-%02d_%03d.png"
                    : "screenshot_%04d-%02d-%02d_%02d-%02d-%02d_%03d.png",
        tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
        tmv.tm_hour, tmv.tm_min, tmv.tm_sec, (int)ms.count());

    std::string root = sf_log::ResolveProjectRoot();
    std::string dir = root + "/screenshots";
#if __has_include(<filesystem>)
    std::error_code ec;
    sf_shot_fs::create_directories(sf_shot_fs::path(dir), ec);
#endif
    return dir + "/" + file;
}

bool WritePngRGBA(const char* path, std::vector<uint8>& pixels, int w, int h)
{
    if (!path || w <= 0 || h <= 0 || pixels.empty()) return false;
    FlipRowsRGBA(pixels, w, h);
    return stbi_write_png(path, w, h, 4, pixels.data(), w * 4) != 0;
}

} // namespace sf_screenshot
