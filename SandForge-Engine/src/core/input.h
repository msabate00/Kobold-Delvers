#pragma once
#include "app/module.h"
#include <cstdint>
#include <GLFW/glfw3.h>
#include "material.h"

class Input : public Module {
public:
    Input(App* app, bool start_enabled = true);
    virtual ~Input();

    bool Awake();
    bool Start();
    bool PreUpdate();
    bool Update(float dt);
    bool PostUpdate();

    bool CleanUp();

    void SetupWindow(GLFWwindow* window);

    void BeginFrame();
    void EndFrame();

    void ProcessBindings(Material& brushMat, int& brushSize);

    bool KeyRepeat(int key)   const { return keys[key]; }
    bool KeyDown(int key)   const { return pressed[key]; }
    bool KeyUp(int key)     const { return released[key]; }

    bool MouseDown(int button) const { return mouseDown[button]; }
    bool MouseUp(int button) const { return mouseUp[button]; }

    double MouseX() const;
    double MouseY() const;

    static void SWindowSize(GLFWwindow* w, int ww, int wh);
    static void SFramebufferSize(GLFWwindow* w, int fw, int fh);

    int ScrollSteps() const { return scrollYSteps; }

private:
    static void SKey(GLFWwindow* w, int key, int sc, int action, int mods);
    static void SMouseBtn(GLFWwindow* w, int button, int action, int mods);
    static void SScroll(GLFWwindow* w, double xoff, double yoff);

private:
    GLFWwindow* window = nullptr;

    bool keys[512] = {};
    bool pressed[512] = {};
    bool released[512] = {};

    bool mouseDown[8] = {};
    bool mouseUp[8] = {};

    double mx = 0.0, my = 0.0;
    double scrollYAccum = 0.0;
    int    scrollYSteps = 0;
};
