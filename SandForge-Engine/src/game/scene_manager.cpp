#include "scene_manager.h"
#include "scenes.h"
#include "app/app.h"
#include "render/renderer.h"


SceneManager::SceneManager(App* app, bool start_enabled) : Module(app, start_enabled) {};
SceneManager::~SceneManager() = default;

bool SceneManager::Awake()
{
    scenes[SCENE_MAINMENU] = new Scene_MainMenu(app, this);
    scenes[SCENE_LEVELSELECTOR] = new Scene_LevelSelector(app, this);
    scenes[SCENE_TUTORIAL] = new Scene_Level(app, this, SCENE_TUTORIAL, "levels/vanilla/level_003.lvl");
    scenes[SCENE_SANDBOX] = new Scene_Sandbox(app, this);
    scenes[SCENE_LEVEL1] = new Scene_Level(app, this, SCENE_LEVEL1, "levels/vanilla/level_001.lvl");
    scenes[SCENE_LEVEL2] = new Scene_Level(app, this, SCENE_LEVEL2, "levels/vanilla/level_002.lvl");

    currentId = SCENE_MAINMENU;
    pendingId = currentId;
    hasPending = false;
    return true;
}

bool SceneManager::Start()
{
    InitFadeSequence(SPRITE_DIR "/fade/fade_%03d.png", 24, 60.0f);

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
        fadePlayer.SetLibrary(&fadeLib);
        fadePhase = FadePhase::Out;
        fadePlayer.Play("fade_out", true);
    }
}

bool SceneManager::WorldActive() const
{
    Scene* s = scenes[currentId];
    return s ? s->HasWorld() : false;
}

void SceneManager::ApplyPending()
{
    if (!hasPending) return;

    if (fadeEnabled && fadePhase == FadePhase::Out && !fadePlayer.IsFinished()) return;

    Scene* cur = scenes[currentId];
    if (cur) cur->OnExit();

    currentId = pendingId;

    Scene* nxt = scenes[currentId];
    if (nxt) nxt->OnEnter();

    hasPending = false;
}

bool SceneManager::InitFadeSequence(const char* pathPattern, int count, float fps)
{
    fadeEnabled = false;
    fadePhase = FadePhase::None;
    fadeLib.Clear();
    fadeFrames.clear();

    if (!pathPattern || !pathPattern[0] || count <= 0) return false;

    fadeFrames.reserve((size_t)count);
    for (int i = 1; i < count; ++i) {
        char path[512];
        std::snprintf(path, sizeof(path), pathPattern, i);
        fadeFrames.emplace_back();
        if (!fadeFrames.back().Load(path)) {
            fadeFrames.pop_back();
            break;
        }
    }

    if (fadeFrames.empty()) return false;

    auto& out = fadeLib.Add("fade_out");
    out.fps = fps;
    out.loop = AnimLoopMode::None;
    out.frames.reserve(fadeFrames.size());
    for (auto& t : fadeFrames) {
        SpriteAnimFrame f;
        f.tex = &t;
        f.uv = UVRect{ 0,0,1,1 };
        out.frames.push_back(f);
    }

    auto& in = fadeLib.Add("fade_in");
    in.fps = fps;
    in.loop = AnimLoopMode::None;
    in.frames.reserve(fadeFrames.size());
    for (int i = (int)fadeFrames.size() - 1; i >= 0; --i) {
        SpriteAnimFrame f;
        f.tex = &fadeFrames[(size_t)i];
        f.uv = UVRect{ 0,0,1,1 };
        in.frames.push_back(f);
    }

    fadePlayer.SetLibrary(&fadeLib);
    fadeSprite.layer = RenderLayer::UI;
    fadeSprite.sort = 1000;
    fadeSprite.color = 0xFFFFFFFF;
    fadeEnabled = true;
    return true;
}

void SceneManager::UpdateFade(float dt)
{
    if (!fadeEnabled || fadePhase == FadePhase::None) return;

    fadePlayer.Update(dt);

    fadeSprite.x = 0.0f;
    fadeSprite.y = 0.0f;
    fadeSprite.w = (float)app->framebufferSize.x;
    fadeSprite.h = (float)app->framebufferSize.y;
    fadePlayer.ApplyTo(fadeSprite, false);
    app->renderer->Queue(fadeSprite);

    if (fadePhase == FadePhase::Out && fadePlayer.IsFinished()) {
        fadePhase = FadePhase::In;
        fadePlayer.Play("fade_in", true);
    }
    else if (fadePhase == FadePhase::In && fadePlayer.IsFinished()) {
        fadePhase = FadePhase::None;
    }
}