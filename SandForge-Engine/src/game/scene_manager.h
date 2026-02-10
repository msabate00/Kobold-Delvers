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
    SceneId CurrentId() const { return currentId; }
    bool WorldActive() const;

private:
    void ApplyPending();

private:
    Scene* scenes[SCENE_COUNT] = {};
    SceneId currentId = SCENE_MAINMENU;
    SceneId pendingId = SCENE_MAINMENU;
    bool hasPending = false;
};
