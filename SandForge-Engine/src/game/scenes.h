#pragma once

#include "scene.h"

#include <string>
#include <vector>

class SceneManager;

//MAIN MENU
class Scene_MainMenu : public Scene {
public:
    Scene_MainMenu(App* app, SceneManager* mgr) : Scene(app), mgr(mgr) {}
    SceneId GetId() const override { return SCENE_MAINMENU; }
    bool HasWorld() const override { return false; }
    void DrawUI(int& brushSize, Material& brushMat) override;
private:
    SceneManager* mgr = nullptr;
};

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

//NIVEL ESTANDARD
class Scene_Level : public Scene {
public:
    Scene_Level(App* app, SceneManager* mgr, SceneId id, const char* levelPath)
        : Scene(app), mgr(mgr), id(id), levelPath(levelPath ? levelPath : "") {}

    SceneId GetId() const override { return id; }
    bool HasWorld() const override { return true; }

    void OnEnter() override;
    void Update(float dt) override;
    void DrawUI(int& brushSize, Material& brushMat) override;

protected:
    SceneManager* mgr = nullptr;
    SceneId id;
    std::string levelPath;
};

//SANDBOX
class Scene_Sandbox : public Scene_Level {
public:
    Scene_Sandbox(App* app, SceneManager* mgr);

    SceneId GetId() const override { return SCENE_SANDBOX; }
    void OnEnter() override;
    void Update(float dt) override;
    void DrawUI(int& brushSize, Material& brushMat) override;

private:
    void ScanLevels();
    void EnsureLevelsFolder();
    std::string NextAutoName();

private:
    std::vector<std::string> files;
    int selected = 0;
    bool requestRescan = true;
};
