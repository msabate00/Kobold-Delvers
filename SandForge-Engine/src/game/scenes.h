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
    void OnEnter() override;
    void Update(float dt) override;
    void DrawUI(int& brushSize, Material& brushMat) override;

private:
    bool DrawMainMenuButton(float x, float y, float w, float h, const char* text, uint32 color, float delay, float introDir, float& hoverT);

private:
    SceneManager* mgr = nullptr;
    float uiIntroTimer = 0.0f;
    float logoBobTimer = 0.0f;
    float levelsHoverT = 0.0f;
    float sandboxHoverT = 0.0f;
    float settingsHoverT = 0.0f;
    float quitHoverT = 0.0f;
};

//NIVEL ESTANDARD
class Scene_Level : public Scene {
public:
    Scene_Level(App* app, SceneManager* mgr, SceneId id, const char* levelPath, const char* levelName = "")
        : Scene(app), mgr(mgr), id(id), levelPath(levelPath ? levelPath : ""), levelName(levelName) { }

    SceneId GetId() const override { return id; }
    bool HasWorld() const override { return true; }

    std::vector<Material> GetSceneMaterials();

    bool IsSpecialLevel() const;
    int LevelStarSlots() const;

    void OnEnter() override;
    void OnExit() override;
    void Update(float dt) override;
    void DrawUI(int& brushSize, Material& brushMat) override;

protected:
    int LevelIndex() const;
    void RestartLevel();
    void CheckLevelCompleted();
    bool PlayerTouchesGoal() const;
    void DrawMaterialBudgetBar();
    void DrawLevelCompleteModal();

protected:
    SceneManager* mgr = nullptr;
    SceneId id;
    std::string levelPath;
    std::string levelName;

    bool levelFinished = false;
    bool levelResultSaved = false;
    bool bonusStarEarned = false;
    bool budgetStarEarned = false;
    uint8 levelStarsEarned = 0;
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
