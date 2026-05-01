#pragma once

#include "scene.h"

#include "render/anim.h"
#include "render/texture.h"
#include "core/level.h"
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
    bool DrawMainMenuButton(float x, float y, float w, float h, const char* text, uint32 color, float delay, float introDir, float& hoverT, AtlasRectPx upRect, AtlasRectPx downRect);
    void ResetBgNpc(int i, float vw, float vh, bool firstTime);
    void ResetBgMat(int i, float vw, float vh, bool firstTime);
    void DrawMenuBackground(float vw, float vh);

private:
    struct BgNpc {
        float x = 0.0f;
        float y = 0.0f;
        float baseX = 0.0f;
        float speed = 0.0f;
        float size = 1.0f;
        float alpha = 255.0f;
        SpriteAnimPlayer anim;
        float swayT = 0.0f;
        float swayAmp = 0.0f;
        float swaySpeed = 0.0f;
        int respawn = 0;
    };

    struct BgMat {
        float x = 0.0f;
        float y = 0.0f;
        float baseX = 0.0f;
        float speed = 0.0f;
        float size = 24.0f;
        float alpha = 255.0f;
        float swayT = 0.0f;
        float swayAmp = 0.0f;
        float swaySpeed = 0.0f;
        Material mat = Material::Sand;
        int respawn = 0;
    };

    SceneManager* mgr = nullptr;
    float uiIntroTimer = 0.0f;
    float logoBobTimer = 0.0f;
    float levelsHoverT = 0.0f;
    float compendiumHoverT = 0.0f;
    float sandboxHoverT = 0.0f;
    float settingsHoverT = 0.0f;
    float quitHoverT = 0.0f;

    Texture2D menuNpcTex;
    SpriteAnimLibrary menuNpcAnims;
    Texture2D logoTex;
    bool logoLoaded = false;

    std::vector<BgNpc> bgNpcs;
    std::vector<BgMat> bgMats;
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

    void ResetUndo();
    void SaveUndo();
    void LoadUndo(int index);
    void Undo();
    void Redo();
    void UpdateUndoInput();

private:
    static const int MAX_UNDO = 20;

    std::vector<std::string> files;
    int selected = 0;
    bool requestRescan = true;

    std::vector<Level> undoLevels;
    int undoIndex = -1;
    bool painting = false;
};
