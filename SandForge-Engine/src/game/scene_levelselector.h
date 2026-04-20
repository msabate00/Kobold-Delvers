#pragma once
#include "scene.h"
#include <array>

class SceneManager;

//SELECTOR DE NIVELES
class Scene_LevelSelector : public Scene {
public:
    Scene_LevelSelector(App* app, SceneManager* mgr) : Scene(app), mgr(mgr) {}
    SceneId GetId() const override { return SCENE_LEVELSELECTOR; }
    bool HasWorld() const override { return false; }
    void OnEnter() override;
    void DrawUI(int& brushSize, Material& brushMat) override;
    void Update(float dt) override;
private:
    bool DrawTopButton(float x, float y, float w, float h, const char* text, uint32 color, float delay, float introDir, float& hoverT);

private:
    SceneManager* mgr = nullptr;
    float uiIntroTimer = 0.0f;
    float backHoverT = 0.0f;
    float settingsHoverT = 0.0f;
    std::array<float, 12> levelHoverT{};
};
