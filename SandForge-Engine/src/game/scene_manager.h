#pragma once
#include "app/module.h"
#include "scene.h"
#include "render/anim.h"

class SceneManager : public Module {
public:
    SceneManager(App* app, bool start_enabled = true);
    ~SceneManager();

    bool Awake();
    bool Start();
    bool CleanUp();

    void BeginFrame();
    bool Update(float dt);
    void DrawUI(int& brushSize, Material& brushMat);

    void Request(SceneId id);
    SceneId CurrentId() const { return currentId; }
    bool WorldActive() const;

private:
    void ApplyPending();
    void UpdateFade(float dt);
    bool InitFadeSequence(const char* pathPattern, int count, float fps);

private:
    Scene* scenes[SCENE_COUNT] = {};
    SceneId currentId = SCENE_MAINMENU;
    SceneId pendingId = SCENE_MAINMENU;
    bool hasPending = false;

    bool fadeEnabled = false;
    enum class FadePhase : uint8 { None = 0, Out, In };
    FadePhase fadePhase = FadePhase::None;
    SpriteAnimLibrary fadeLib;
    SpriteAnimPlayer fadePlayer;
    std::vector<Texture2D> fadeFrames;
    Sprite fadeSprite;
};
