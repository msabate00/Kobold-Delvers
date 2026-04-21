#pragma once

#include "app/module.h"
#include "app/defs.h"

class GLFWwindow;

enum class WindowMode : uint8 {
    Windowed = 0,
    Fullscreen,
    Borderless
};

struct SettingsData {
    float musicVolume = 0.1f;
    float sfxVolume   = 0.5f;
    bool  vsync       = true;
    WindowMode windowMode = WindowMode::Windowed;
    int windowW = 1280;
    int windowH = 720;
};

class Settings : public Module {
public:
    Settings(App* app, bool start_enabled = true);
    ~Settings() override = default;

    bool Awake() override;
    bool CleanUp() override { return true; }

    bool Load(const char* path = "settings.cfg");
    bool Save(const char* path = "settings.cfg");

    void ApplyGraphics();
    void ApplyAudio();
    void ResetToDefaults();

public:
    SettingsData data;

private:
    GLFWwindow* Window() const;

private:
    bool loaded = false;
    WindowMode currentWindowMode = WindowMode::Windowed;

    // Runtime windowed rect cache (NOT persisted).
    int lastWindowX = 100;
    int lastWindowY = 100;
    int lastWindowW = 1280;
    int lastWindowH = 720;
};
