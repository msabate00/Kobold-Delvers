#pragma once
#include "app/module.h"
#include "scene.h"

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
    void OpenSettings(SceneId returnTo);
    SceneId SettingsReturnTo() const { return settingsReturnTo; }
    SceneId CurrentId() const { return currentId; }
    bool WorldActive() const;

    void ResetScene();

    float Fade() const { return fade; }

private:
    void ApplyPending();
    void UpdateFade(float dt);
    void SyncSceneMusic(SceneId id, float fadeSec);

private:
    Scene* scenes[SCENE_COUNT] = {};
    SceneId currentId = SCENE_MAINMENU;
    SceneId pendingId = SCENE_MAINMENU;
    bool hasPending = false;

    bool fadeEnabled = true;
    enum class FadePhase : uint8 { None = 0, Out, In };
    FadePhase fadePhase = FadePhase::None;

    //Fade config
    float fade = 0.0f;
    float fadeOutSec = 0.2f;
    float fadeInSec  = 0.2f;
    SceneId settingsReturnTo = SCENE_MAINMENU;
};
