#pragma once
#include "scene.h"

class SceneManager;

class Scene_Settings : public Scene {
public:
    Scene_Settings(App* app, SceneManager* mgr) : Scene(app), mgr(mgr) {}
    SceneId GetId() const override { return SCENE_SETTINGS; }
    bool HasWorld() const override { return false; }

    void OnEnter() override;
    void Update(float dt) override;
    void DrawUI(int& brushSize, Material& brushMat) override;

private:
    SceneManager* mgr = nullptr;
};
