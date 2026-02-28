#include "npc_system.h"
#include "worldsim.h"
#include "engine.h"
#include "app/app.h"
#include "render/renderer.h"
#include <algorithm>
#include <cmath>

bool NPCSystem::Awake(const WorldSim& world)
{
    const size_t n = (size_t)world.GridW() * (size_t)world.GridH();
    occ.assign(n, 0);
    return true;
}

bool NPCSystem::Start()
{
    npcTex.Load(SPRITE_DIR "/KoboldMiner.png");

    npcAnims.Clear();
    {
        auto& idle = npcAnims.Add("idle");
        idle.defaultTex = &npcTex;
        idle.fps = 6.0f;
        idle.loop = AnimLoopMode::Loop;
        idle.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,0,12,12 }, 0.1f));

        auto& walk = npcAnims.Add("walk");
        walk.defaultTex = &npcTex;
        walk.fps = 6.0f;
        walk.loop = AnimLoopMode::Loop;
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,12,12,12 }, 0.1f));
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,12,12,12 }, 0.1f));
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,12,12,12 }, 0.1f));
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 36,12,12,12 }, 0.1f));
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 48,12,12,12 }, 0.1f));

        auto& fall = npcAnims.Add("fall");
        fall.defaultTex = &npcTex;
        fall.fps = 6.0f;
        fall.loop = AnimLoopMode::PingPong;
        fall.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,24,12,12 }, 0.1f));
        fall.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,24,12,12 }, 0.1f));
        fall.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,24,12,12 }, 0.1f));

        auto& die = npcAnims.Add("die");
        die.defaultTex = &npcTex;
        die.fps = 6.0f;
        die.loop = AnimLoopMode::None;
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 36,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 48,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 60,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,0,0,0 }, 0.1f));
    }

    return true;
}

void NPCSystem::Clear(const WorldSim& world)
{
    npcs.clear();
    const size_t n = (size_t)world.GridW() * (size_t)world.GridH();
    occ.assign(n, 0);
    npcMoveAcc = 0.0f;
}

NPC& NPCSystem::AddNPC(WorldSim& world, int x, int y, int dir)
{
    //Visual
    const int w = 12;
    const int h = 12;

    // Hitbox
    const int padX = 3;
    const int padTop = 5;
    const int padBottom = 0;

    NPC npc{};
    npc.x = x;
    npc.y = y;
    npc.w = w;
    npc.h = h;
    npc.dir = dir;
    npc.alive = true;

    npc.hbOffX = std::clamp(padX, 0, std::max(0, w - 1));
    npc.hbOffY = std::clamp(padTop, 0, std::max(0, h - 1));
    npc.hbW = std::max(1, w - 2 * padX);
    npc.hbH = std::max(1, h - padTop - padBottom);

    npc.sprite.tex = &npcTex;
    npc.anim.SetLibrary(&npcAnims);
    npc.anim.Play("idle", true);
    npc.anim.ApplyTo(npc.sprite, npc.dir < 0);
    npcs.push_back(npc);

    world.MarkChunksInRect(x, y, w, h);

    return npcs.back();
}

void NPCSystem::RebuildOcc(const WorldSim& world)
{
    if (occ.size() != (size_t)world.GridW() * (size_t)world.GridH())
        occ.assign((size_t)world.GridW() * (size_t)world.GridH(), 0);
    else
        std::fill(occ.begin(), occ.end(), 0);

    for (int i = 0; i < (int)npcs.size(); ++i) {
        const auto& n = npcs[i];
        if (!n.alive) continue;
        const int bx = n.x + n.hbOffX;
        const int by = n.y + n.hbOffY;
        for (int yy = 0; yy < n.hbH; ++yy)
            for (int xx = 0; xx < n.hbW; ++xx) {
                const int gx = bx + xx;
                const int gy = by + yy;
                if (world.InRange(gx, gy)) occ[world.LinearIndex(gx, gy)] = i + 1;
            }
    }
}

bool NPCSystem::RectFreeOnBack(const WorldSim& world, int x, int y, int w, int h, int ignoreId) const
{
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx) {
            const int gx = x + xx;
            const int gy = y + yy;
            if (!world.InRange(gx, gy)) return false;
            const int i = world.LinearIndex(gx, gy);
            if (world.BackMatAtIndex(i) != (uint8)Material::Empty) return false;
            const int occId = occ.empty() ? 0 : occ[i];
            if (occId != 0 && occId != ignoreId) return false;
        }
    return true;
}

bool NPCSystem::CheckNPCDie(const WorldSim& world, int x, int y, int w, int h) const
{
    auto isLava = [&](int gx, int gy) -> bool {
        if (!world.InRange(gx, gy)) return false;
        switch (world.BackMatAtIndex(world.LinearIndex(gx, gy)))
        {
        case (uint8)Material::Lava: return true;
        case (uint8)Material::Fire: return true;
        default: return false;
        }
    };

    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx)
            if (isLava(x + xx, y + yy)) return true;

    for (int xx = 0; xx < w; ++xx) {
        if (isLava(x + xx, y - 1)) return true;
        if (isLava(x + xx, y + h)) return true;
    }
    for (int yy = 0; yy < h; ++yy) {
        if (isLava(x - 1, y + yy)) return true;
        if (isLava(x + w, y + yy)) return true;
    }

    return false;
}

void NPCSystem::MoveNPCs(WorldSim& world, float fixedTimeStep)
{
    npcMoveAcc += fixedTimeStep;
    const float npcMoveInterval = 1.0f / std::max(0.001f, npcCellsPerSec);
    if (npcMoveAcc < npcMoveInterval) return;
    npcMoveAcc -= npcMoveInterval;

    constexpr int kMaxStep = 1;
    for (int i = 0; i < (int)npcs.size(); ++i) {
        auto& n = npcs[i];
        if (!n.alive) continue;
        if (n.anim.CurrentName() == "die") continue;
        const int id = i + 1;

        const int hbX = n.x + n.hbOffX;
        const int hbY = n.y + n.hbOffY;

        if (CheckNPCDie(world, hbX, hbY, n.hbW, n.hbH)) {
            n.anim.Play("die", false);
            continue;
        }

        if (RectFreeOnBack(world, hbX, hbY + 1, n.hbW, n.hbH, id)) {
            n.y += 1;
            n.anim.Play("fall", false);
            continue;
        }

        const int nx = n.x + n.dir;

        if (RectFreeOnBack(world, nx + n.hbOffX, hbY, n.hbW, n.hbH, id)) {
            n.x = nx;
            n.anim.Play("walk", false);
            continue;
        }

        auto solid = [&](int gx, int gy) {
            return world.InRange(gx, gy) && world.BackMatAtIndex(world.LinearIndex(gx, gy)) != (uint8)Material::Empty;
        };

        bool climbed = false;
        for (int step = 1; step <= kMaxStep; ++step) {
            if (!RectFreeOnBack(world, nx + n.hbOffX, (n.y - step) + n.hbOffY, n.hbW, n.hbH, id)) continue;

            bool support = false;
            const int supY = (n.y - step) + n.hbOffY + n.hbH;
            for (int xx = 0; xx < n.hbW; ++xx)
                support |= solid((nx + n.hbOffX) + xx, supY);
            if (!support) continue;
            n.y -= step;
            n.x = nx;
            climbed = true;
            break;
        }
        if (!climbed) n.dir = -n.dir;
    }
}

void NPCSystem::AnimateNPCs(Engine& engine, float dt)
{
    App* app = engine.app;
    if (!app || !app->renderer) return;

    for (auto& n : npcs) {
        n.anim.Update(dt);
        n.anim.ApplyTo(n.sprite, n.dir < 0);

        if (n.anim.CurrentName() == "die" && n.anim.IsFinished()) {
            n.alive = false;
        }

        float sx, sy, sw, sh;
        if (!engine.WorldRectToScreen((float)n.x, (float)n.y, (float)n.w, (float)n.h,
            app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh))
            continue;

        n.sprite.x = std::floor(sx);
        n.sprite.y = std::floor(sy);
        n.sprite.w = std::floor(sw);
        n.sprite.h = std::floor(sh);
        n.sprite.layer = RenderLayer::WORLD;
        app->renderer->Queue(n.sprite);
    }
}
