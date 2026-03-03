#pragma once

#include "app/defs.h"
#include "core/material.h"

class App;

enum SceneId : int {
    SCENE_MAINMENU = 0,
    SCENE_SETTINGS,
    SCENE_LEVELSELECTOR,
    SCENE_TUTORIAL,
    SCENE_SANDBOX,
    SCENE_LEVEL1,
    SCENE_LEVEL2,
    SCENE_LEVEL3,
    SCENE_LEVEL4,
    SCENE_LEVEL5,
    SCENE_LEVEL6,
    SCENE_LEVEL7,
    SCENE_LEVEL8,
    SCENE_LEVEL9,
    SCENE_LEVEL10,
    SCENE_LEVEL11,
    SCENE_LEVEL12,
    SCENE_LEVEL13,
    SCENE_LEVEL14,
    SCENE_LEVEL15,

    SCENE_COUNT
};

class Scene {
public:
    Scene(App* app) : app(app) {}
    virtual ~Scene() {}

    virtual SceneId GetId() const = 0;

    //Simulacion granular
    virtual bool HasWorld() const { return false; }

    virtual void OnEnter() {}
    virtual void OnExit() {}

    virtual void Update(float dt) {}

    //Draw ahora lo llevan las escenas
    virtual void DrawUI(int& brushSize, Material& brushMat) {}

protected:
    App* app = nullptr;
};
