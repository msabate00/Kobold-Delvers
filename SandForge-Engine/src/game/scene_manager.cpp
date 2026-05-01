#include "scene_manager.h"
#include "scenes.h"
#include "scene_settings.h"
#include "scene_levelselector.h"
#include "scene_compendium.h"
#include "app/app.h"
#include "audio/audio.h"
#include "render/renderer.h"
#include <cmath>


SceneManager::SceneManager(App* app, bool start_enabled) : Module(app, start_enabled) {};
SceneManager::~SceneManager() = default;

bool SceneManager::Awake()
{
    scenes[SCENE_MAINMENU] = new Scene_MainMenu(app, this);
    scenes[SCENE_LEVELSELECTOR] = new Scene_LevelSelector(app, this);
    scenes[SCENE_COMPENDIUM] = new Scene_Compendium(app, this);
    scenes[SCENE_TUTORIAL] = new Scene_Level(app, this, SCENE_TUTORIAL, "levels/vanilla/tutorial.lvl");
    scenes[SCENE_SANDBOX] = new Scene_Sandbox(app, this);
    scenes[SCENE_LEVEL1] = new Scene_Level(app, this, SCENE_LEVEL1, "levels/vanilla/level_01.lvl", "FIRST STEPS");
    scenes[SCENE_LEVEL2] = new Scene_Level(app, this, SCENE_LEVEL2, "levels/vanilla/level_02.lvl", "LAVA BURNS");
    scenes[SCENE_LEVEL3]  = new Scene_Level(app, this, SCENE_LEVEL3,  "levels/vanilla/level_03.lvl", "LAKE MOVEMENT");
    scenes[SCENE_LEVEL4]  = new Scene_Level(app, this, SCENE_LEVEL4,  "levels/vanilla/level_04_special.lvl", "YOU ARE KOBOLD 1"); //especial
    scenes[SCENE_LEVEL5]  = new Scene_Level(app, this, SCENE_LEVEL5,  "levels/vanilla/level_05.lvl", "PILLARS IN LAVA");
    scenes[SCENE_LEVEL6]  = new Scene_Level(app, this, SCENE_LEVEL6,  "levels/vanilla/level_06.lvl", "ANCIENT MINE");
    scenes[SCENE_LEVEL7]  = new Scene_Level(app, this, SCENE_LEVEL7,  "levels/vanilla/level_07.lvl", "THE FALL");
    scenes[SCENE_LEVEL8]  = new Scene_Level(app, this, SCENE_LEVEL8,  "levels/vanilla/level_08_special.lvl", "YOU ARE KOBOLD 2"); //especial
    scenes[SCENE_LEVEL9]  = new Scene_Level(app, this, SCENE_LEVEL9,  "levels/vanilla/level_09.lvl", "THE CLIMB");
    scenes[SCENE_LEVEL10] = new Scene_Level(app, this, SCENE_LEVEL10, "levels/vanilla/level_10.lvl", "WE'RE... LOST?");
    scenes[SCENE_LEVEL11] = new Scene_Level(app, this, SCENE_LEVEL11, "levels/vanilla/level_11.lvl", "IT'S VERY HOT");
    scenes[SCENE_LEVEL12] = new Scene_Level(app, this, SCENE_LEVEL12, "levels/vanilla/level_12_special.lvl", "YOU ARE KOBOLD 3"); //especial
    scenes[SCENE_SETTINGS] = new Scene_Settings(app, this);

    currentId = SCENE_MAINMENU;
    pendingId = currentId;
    hasPending = false;
    return true;
}

bool SceneManager::Start()
{
    if (scenes[currentId]) scenes[currentId]->OnEnter();
    SyncSceneMusic(currentId, 0.0f);
    return true;
}

bool SceneManager::CleanUp()
{
    // Make sure current leaves
    if (scenes[currentId]) scenes[currentId]->OnExit();

    for (int i = 0; i < SCENE_COUNT; ++i) {
        delete scenes[i];
        scenes[i] = nullptr;
    }
    return true;
}

void SceneManager::BeginFrame()
{
    ApplyPending();
}

bool SceneManager::Update(float dt)
{
    Scene* s = scenes[currentId];
    if (s) s->Update(dt);

    UpdateFade(dt);
    ApplyPending();
    return true;
}

void SceneManager::DrawUI(int& brushSize, Material& brushMat)
{
    Scene* s = scenes[currentId];
    if (s) s->DrawUI(brushSize, brushMat);
    ApplyPending();
}

void SceneManager::Request(SceneId id)
{
    if (id < 0 || id >= SCENE_COUNT) return;
    if (id == currentId) return;

    pendingId = id;
    hasPending = true;

    if (fadeEnabled && fadePhase == FadePhase::None) {
        fadePhase = FadePhase::Out;
    }
}

void SceneManager::OpenSettings(SceneId returnTo)
{
    settingsReturnTo = returnTo;
    Request(SCENE_SETTINGS);
}


bool SceneManager::WorldActive() const
{
    Scene* s = scenes[currentId];
    return s ? s->HasWorld() : false;
}

void SceneManager::ResetScene()
{
    if (currentId < SCENE_LEVEL1 || currentId > SCENE_LEVEL12) return;
    pendingId = currentId;
    hasPending = true;

    if (fadeEnabled && fadePhase == FadePhase::None) {
        fadePhase = FadePhase::Out;
    }
}

void SceneManager::SyncSceneMusic(SceneId id, float fadeSec)
{
    if (!app || !app->audio) return;

    const char* key = nullptr;
    switch (id) {
    case SCENE_MAINMENU:
    case SCENE_LEVELSELECTOR:
    case SCENE_COMPENDIUM:
        key = "main_menu";
        break;
    case SCENE_SANDBOX:
        key = "sandbox";
        break;
    case SCENE_SETTINGS:
        return;
    default:
        key = "in_game";
        break;
    }

    app->audio->PlayMusic(key, fadeSec);
}

void SceneManager::ApplyPending()
{
    if (!hasPending) return;

    if (fadeEnabled && fadePhase == FadePhase::Out && fade < 0.999f) return;

    Scene* cur = scenes[currentId];
    if (cur) cur->OnExit();

    currentId = pendingId;

    Scene* nxt = scenes[currentId];
    if (nxt) nxt->OnEnter();

    SyncSceneMusic(currentId, fadeEnabled ? 0.35f : 0.0f);

    hasPending = false;

    if (fadeEnabled && fadePhase == FadePhase::Out) {
        fadePhase = FadePhase::In;
    }
}

void SceneManager::UpdateFade(float dt)
{
    if (!fadeEnabled) {
        fade = 0.0f;
        fadePhase = FadePhase::None;
        app->screenFade = 0.0f;
        return;
    }

    if (fadePhase == FadePhase::Out) {
        fade += dt / fadeOutSec;
        if (fade > 1.0f) fade = 1.0f;
    }
    else if (fadePhase == FadePhase::In) {
        fade -= dt / fadeInSec;
        if (fade < 0.0f) fade = 0.0f;
        if (fade <= 0.0f) fadePhase = FadePhase::None;
    }

    app->screenFade = fade;
}