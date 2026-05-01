#pragma once
#include "scene.h"
#include <vector>

class SceneManager;

//COMPENDIO / BIBLIOTECA
class Scene_Compendium : public Scene {
public:
    Scene_Compendium(App* app, SceneManager* mgr) : Scene(app), mgr(mgr) {}
    SceneId GetId() const override { return SCENE_COMPENDIUM; }
    bool HasWorld() const override { return false; }
    void OnEnter() override;
    void Update(float dt) override;
    void DrawUI(int& brushSize, Material& brushMat) override;

public:
    struct MiniCell {
        int x = 1;
        int y = 1;
        Material mat = Material::Null;
    };

    struct MatInteraction {
        const char* text = "";
        std::vector<std::vector<MiniCell>> steps;
    };

    struct CompendiumEntry {
        const char* name = "";
        Material mat = Material::Empty;
        const char* desc = "";
        int unlockLevel = 0;
        std::vector<MatInteraction> interactions;
    };

private:
    void CompendiumEntries();

    bool DrawTopButton(float x, float y, float w, float h, const char* text, uint32 color, float& hoverT);
    void DrawEntryButton(int index, float x, float y, float w, float h, float scale);
    void DrawInfoPanel(const CompendiumEntry& e, float x, float y, float w, float h, float scale);
    void DrawMatIcon(Material mat, float x, float y, float size, uint32 tint);
    void DrawHolder(float x, float y, float size, uint32 tint);
    void DrawGrid(float x, float y, float size, const std::vector<MiniCell>& cells, uint32 tint);
    void DrawInteraction(const MatInteraction& it, float x, float y, float w, float scale);

    bool EntryUnlocked(const CompendiumEntry& e) const;
    int FindFirstUnlocked() const;

private:
    SceneManager* mgr = nullptr;
    std::vector<CompendiumEntry> entries;
    float uiIntroTimer = 0.0f;
    float backHoverT = 0.0f;
    float settingsHoverT = 0.0f;
    float listScroll = 0.0f;
    float infoScroll = 0.0f;
    int selectedEntry = 0;
    std::vector<float> entryHoverT;
};
