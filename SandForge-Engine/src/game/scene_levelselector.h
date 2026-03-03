#pragma once
#include "scene.h"

class SceneManager;

//SELECTOR DE NIVELES
class Scene_LevelSelector : public Scene {
public:
    Scene_LevelSelector(App* app, SceneManager* mgr) : Scene(app), mgr(mgr) {}
    SceneId GetId() const override { return SCENE_LEVELSELECTOR; }
    bool HasWorld() const override { return false; }
    void DrawUI(int& brushSize, Material& brushMat) override;
    void Update(float dt) override;
private:
    SceneManager* mgr = nullptr;
};
