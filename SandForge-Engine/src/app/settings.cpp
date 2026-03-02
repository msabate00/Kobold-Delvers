#include "app/settings.h"

#include "app/app.h"
#include "audio/audio.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>


Settings::Settings(App* app, bool start_enabled) : Module(app, start_enabled) {}


GLFWwindow* Settings::Window() const
{
    return app ? app->GetWindow() : nullptr;
}


static inline std::string Trim(const std::string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

static inline bool ParseBool(const std::string& v, bool def)
{
    std::string t = v;
    for (char& c : t) c = (char)std::tolower((unsigned char)c);
    if (t == "1" || t == "true" || t == "yes" || t == "on") return true;
    if (t == "0" || t == "false" || t == "no" || t == "off") return false;
    return def;
}

static inline WindowMode ParseWindowMode(const std::string& v, WindowMode def)
{
    std::string t = v;
    for (char& c : t) c = (char)std::tolower((unsigned char)c);
    if (t == "windowed" || t == "0") return WindowMode::Windowed;
    if (t == "fullscreen" || t == "1") return WindowMode::Fullscreen;
    if (t == "borderless" || t == "2") return WindowMode::Borderless;
    return def;
}

static inline const char* WindowModeToStr(WindowMode m)
{
    switch (m) {
    case WindowMode::Windowed: return "windowed";
    case WindowMode::Fullscreen: return "fullscreen";
    case WindowMode::Borderless: return "borderless";
    default: return "windowed";
    }
}


bool Settings::Load(const char* path)
{
    if (!path || !*path) path = "settings.cfg";

    std::ifstream f(path);
    if (!f) {
        loaded = true;
        // Keep defaults.
        return false;
    }

    std::string line;
    while (std::getline(f, line)) {
        line = Trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string k = Trim(line.substr(0, eq));
        std::string v = Trim(line.substr(eq + 1));
        if (k.empty()) continue;

        try {
            if (k == "musicVolume") data.musicVolume = std::clamp(std::stof(v), 0.0f, 1.0f);
            else if (k == "sfxVolume") data.sfxVolume = std::clamp(std::stof(v), 0.0f, 1.0f);
            else if (k == "vsync") data.vsync = ParseBool(v, data.vsync);
            else if (k == "windowMode") data.windowMode = ParseWindowMode(v, data.windowMode);
            else if (k == "windowW") data.windowW = std::fmax(64, std::stoi(v));
            else if (k == "windowH") data.windowH = std::fmax(64, std::stoi(v));
        }
        catch (...) {
            // Ignore malformed lines.
        }
    }

    // Mirror to runtime cache
    lastWindowW = data.windowW;
    lastWindowH = data.windowH;

    loaded = true;
    return true;
}


bool Settings::Save(const char* path)
{
    if (!path || !*path) path = "settings.cfg";

    GLFWwindow* w = Window();
    if (w && currentWindowMode == WindowMode::Windowed) {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetWindowPos(w, &wx, &wy);
        glfwGetWindowSize(w, &ww, &wh);

        // Position is cached only for runtime restores (NOT persisted).
        lastWindowX = wx;
        lastWindowY = wy;
        lastWindowW = ww;
        lastWindowH = wh;

        data.windowW = ww;
        data.windowH = wh;
    }

    std::ofstream o(path, std::ios::out | std::ios::trunc);
    if (!o) return false;

    o << "# Kobold Delvers settings\n";
    o << "musicVolume=" << data.musicVolume << "\n";
    o << "sfxVolume=" << data.sfxVolume << "\n";
    o << "vsync=" << (data.vsync ? 1 : 0) << "\n";
    o << "windowMode=" << WindowModeToStr(data.windowMode) << "\n";
    o << "windowW=" << data.windowW << "\n";
    o << "windowH=" << data.windowH << "\n";

    return true;
}


void Settings::ApplyGraphics()
{
    GLFWwindow* w = Window();
    if (!w || !app) return;

    // VSync
    glfwSwapInterval(data.vsync ? 1 : 0);

    // Cache windowed rect before leaving it
    if (currentWindowMode == WindowMode::Windowed && data.windowMode != WindowMode::Windowed) {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetWindowPos(w, &wx, &wy);
        glfwGetWindowSize(w, &ww, &wh);
        lastWindowX = wx;
        lastWindowY = wy;
        lastWindowW = ww;
        lastWindowH = wh;
        data.windowW = ww;
        data.windowH = wh;
    }

    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* vm = mon ? glfwGetVideoMode(mon) : nullptr;
    int monX = 0, monY = 0;
    if (mon) glfwGetMonitorPos(mon, &monX, &monY);

    if (data.windowMode == WindowMode::Fullscreen && mon && vm) {
        glfwSetWindowAttrib(w, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(w, mon, 0, 0, vm->width, vm->height, vm->refreshRate);
        currentWindowMode = WindowMode::Fullscreen;
    }
    else if (data.windowMode == WindowMode::Borderless && vm) {
        glfwSetWindowAttrib(w, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(w, nullptr, monX, monY, vm->width, vm->height, 0);
        currentWindowMode = WindowMode::Borderless;
    }
    else {
        glfwSetWindowAttrib(w, GLFW_DECORATED, GLFW_TRUE);
        const int ww = data.windowW > 0 ? data.windowW : lastWindowW;
        const int wh = data.windowH > 0 ? data.windowH : lastWindowH;
        glfwSetWindowMonitor(w, nullptr, lastWindowX, lastWindowY, ww, wh, 0);
        currentWindowMode = WindowMode::Windowed;
        lastWindowW = ww;
        lastWindowH = wh;
    }

    // Refresh cached sizes (callbacks might also update these)
    glfwGetWindowSize(w, &app->windowSize.x, &app->windowSize.y);
    glfwGetFramebufferSize(w, &app->framebufferSize.x, &app->framebufferSize.y);

    const float camW = app->framebufferSize.x / (float)std::fmax(1, app->pixelsPerCell);
    const float camH = app->framebufferSize.y / (float)std::fmax(1, app->pixelsPerCell);
    app->SetCameraRect(app->camera.pos.x, app->camera.pos.y, camW, camH);
}


void Settings::ApplyAudio()
{
    if (!app || !app->audio) return;
    app->audio->SetMusicVolume(data.musicVolume);
    app->audio->SetSfxVolume(data.sfxVolume);
}


void Settings::ResetToDefaults()
{
    SettingsData def;
    def.windowW = lastWindowW;
    def.windowH = lastWindowH;
    data = def;
}


bool Settings::Awake()
{
    if (!loaded) Load();

    // Init runtime cache from the current window (if windowed)
    GLFWwindow* w = Window();
    if (w) {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetWindowPos(w, &wx, &wy);
        glfwGetWindowSize(w, &ww, &wh);
        lastWindowX = wx;
        lastWindowY = wy;
        lastWindowW = ww;
        lastWindowH = wh;
    }

    ApplyGraphics();
    return true;
}
