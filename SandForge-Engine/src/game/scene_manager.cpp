#include "scene_manager.h"
#include "scenes.h"

SceneManager::SceneManager(App* app, bool start_enabled) : Module(app, start_enabled) {};
SceneManager::~SceneManager() = default;

bool SceneManager::Awake()
{
    scenes[SCENE_MAINMENU] = new Scene_MainMenu(app, this);
    scenes[SCENE_LEVELSELECTOR] = new Scene_LevelSelector(app, this);
    scenes[SCENE_TUTORIAL] = new Scene_Level(app, this, SCENE_TUTORIAL, "levels/tutorial.lvl");
    scenes[SCENE_SANDBOX] = new Scene_Sandbox(app, this);
    scenes[SCENE_LEVEL1] = new Scene_Level(app, this, SCENE_LEVEL1, "levels/level1.lvl");
    scenes[SCENE_LEVEL2] = new Scene_Level(app, this, SCENE_LEVEL2, "levels/level2.lvl");

    currentId = SCENE_MAINMENU;
    pendingId = currentId;
    hasPending = false;
    return true;
}

bool SceneManager::Start()
{
    if (scenes[currentId]) scenes[currentId]->OnEnter();
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
}

bool SceneManager::WorldActive() const
{
    Scene* s = scenes[currentId];
    return s ? s->HasWorld() : false;
}

void SceneManager::ApplyPending()
{
    if (!hasPending) return;

    Scene* cur = scenes[currentId];
    if (cur) cur->OnExit();

    currentId = pendingId;

    Scene* nxt = scenes[currentId];
    if (nxt) nxt->OnEnter();

    hasPending = false;
}
