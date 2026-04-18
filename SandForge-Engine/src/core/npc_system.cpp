#include "npc_system.h"

#include "worldsim.h"
#include "engine.h"
#include "app/app.h"
#include "render/renderer.h"

#include <algorithm>
#include <cmath>
#include <ui/ui.h>

static NPC MakeDefaultNPC(const Texture2D* npcTex, const SpriteAnimLibrary* npcAnims,
    int x, int y, int dir)
{
    NPC npc{};

    // Visual
    npc.x = x;
    npc.y = y;
    npc.w = 12;
    npc.h = 12;

    // Hitbox
    const int padX = 3;
    const int padTop = 5;
    const int padBottom = 0;

    npc.hbOffX = std::clamp(padX, 0, (int)std::fmax(0, npc.w - 1));
    npc.hbOffY = std::clamp(padTop, 0, (int)std::fmax(0, npc.h - 1));
    npc.hbW = std::fmax(1, npc.w - 2 * padX);
    npc.hbH = std::fmax(1, npc.h - padTop - padBottom);

    npc.dir = dir;
    npc.alive = true;
    npc.parked = false;
    npc.spawnerId = -1;
    npc.goalId = -1;
    npc.speechTimer = 0.0f;
    npc.speechMessage = -1;
    npc.drowning = false;

    npc.sprite.tex = npcTex;
    npc.anim.SetLibrary(npcAnims);
    npc.anim.Play("idle", true);
    npc.anim.ApplyTo(npc.sprite, npc.dir < 0);

    return npc;
}

static bool RectsOverlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < (bx + bw) && (ax + aw) > bx && ay < (by + bh) && (ay + ah) > by;
}

static bool IsNPCPassableMat(uint8 m)
{
    return m == (uint8)Material::Empty
        || m == (uint8)Material::Water
        || m == (uint8)Material::Oil
        || m == (uint8)Material::Acid
        || m == (uint8)Material::Smoke
        || m == (uint8)Material::Steam
        || m == (uint8)Material::FlammableGas
        || m == (uint8)Material::Vine;
}

static bool IsHazardMat(uint8 m)
{
    return m == (uint8)Material::Lava
        || m == (uint8)Material::Fire
        || m == (uint8)Material::HotCoal
        || m == (uint8)Material::Acid;
}

static bool CircleIntersectsRect(int cx, int cy, int radius, int rx, int ry, int rw, int rh)
{
    const int nearestX = std::clamp(cx, rx, rx + rw - 1);
    const int nearestY = std::clamp(cy, ry, ry + rh - 1);
    const int dx = cx - nearestX;
    const int dy = cy - nearestY;
    return dx * dx + dy * dy <= radius * radius;
}

bool NPCSystem::Awake(const WorldSim& world)
{
    const size_t n = (size_t)world.GridW() * (size_t)world.GridH();
    occ.assign(n, 0);
    return true;
}

bool NPCSystem::Start()
{
    npcTex.Load(SPRITE_DIR "/KoboldMiner.png");
    structuresTex.Load(SPRITE_DIR "/Structures.png");

    npcAnims.Clear();
    {
        auto& idle = npcAnims.Add("idle");
        idle.defaultTex = &npcTex;
        idle.fps = 6.0f;
        idle.loop = AnimLoopMode::Loop;
        idle.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,0,12,12 }, 0.1f));
        idle.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,0,12,12 }, 0.1f));
        idle.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,0,12,12 }, 0.1f));
        idle.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,0,12,12 }, 0.1f));

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

        auto& dieFire = npcAnims.Add("dieFire");
        dieFire.defaultTex = &npcTex;
        dieFire.fps = 6.0f;
        dieFire.loop = AnimLoopMode::None;
        dieFire.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,36,12,12 }, 0.1f));
        dieFire.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,36,12,12 }, 0.1f));
        dieFire.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,36,12,12 }, 0.1f));
        dieFire.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 36,36,12,12 }, 0.1f));
        dieFire.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 48,36,12,12 }, 0.1f));
        dieFire.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 60,36,12,12 }, 0.1f));
        dieFire.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,0,0,0 }, 0.1f));

        auto& dieNormal = npcAnims.Add("dieNormal");
        dieNormal.defaultTex = &npcTex;
        dieNormal.fps = 6.0f;
        dieNormal.loop = AnimLoopMode::None;
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,48,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,48,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,48,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 36,48,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 48,48,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 60,48,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,60,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,60,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,60,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 36,60,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 48,60,12,12 }, 0.1f));
        dieNormal.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 60,60,12,12 }, 0.1f));

        auto& climb = npcAnims.Add("climb");
        climb.defaultTex = &npcTex;
        climb.fps = 6.0f;
        climb.loop = AnimLoopMode::Loop;
        climb.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,0,12,12 }, 0.1f));
    }

    spawnerAnims.Clear();
    {
        auto& idle = spawnerAnims.Add("idle");
        idle.defaultTex = &structuresTex;
        idle.fps = 1.0f;
        idle.loop = AnimLoopMode::Loop;
        idle.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 0,0,12,12 }, 0.1f));
    }

    goalAnims.Clear();
    {
        auto& idle = goalAnims.Add("idle");
        idle.defaultTex = &structuresTex;
        idle.fps = 1.0f;
        idle.loop = AnimLoopMode::Loop;
        idle.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 0,12,12,12 }, 0.1f));
    }
    bonusAnims.Clear();
    {
        auto& idle = bonusAnims.Add("idle");
        idle.defaultTex = &structuresTex;
        idle.fps = 1.0f;
        idle.loop = AnimLoopMode::Loop;
        idle.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 0,24,12,12 }, 0.1f));

        auto& lighted = bonusAnims.Add("lighted");
        lighted.defaultTex = &structuresTex;
        lighted.fps = 1.0f;
        lighted.loop = AnimLoopMode::Loop;
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 12,24,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 24,24,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 36,24,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 48,24,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 60,24,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 0,36,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 12,36,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 24,36,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 36,36,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 48,36,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 60,36,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 0,48,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 12,48,12,12 }, 0.1f));
        lighted.frames.push_back(AnimFramePx(&structuresTex, AtlasRectPx{ 24,48,12,12 }, 0.1f)); 
    }

    return true;
}

void NPCSystem::Clear(const WorldSim& world)
{
    npcs.clear();
    spawners.clear();
    goals.clear();
    bonuses.clear();

    const size_t n = (size_t)world.GridW() * (size_t)world.GridH();
    occ.assign(n, 0);
    npcMoveAcc = 0.0f;
}

NPC& NPCSystem::AddNPC(WorldSim& world, int x, int y, int dir)
{
    NPC npc = MakeDefaultNPC(&npcTex, &npcAnims, x, y, dir);
    npcs.push_back(npc);

    world.MarkChunksInRect(x, y, npc.w, npc.h);

    return npcs.back();
}

NPCSpawner& NPCSystem::AddSpawner(WorldSim& world, int x, int y)
{
    NPCSpawner sp{};
    sp.x = x;
    sp.y = y;
    sp.w = 12;
    sp.h = 12;
    sp.maxAlive = 5;
    sp.spawnCooldown = 0.5f;
    sp.spawnAcc = sp.spawnCooldown;

    sp.sprite.tex = &structuresTex;
    sp.anim.SetLibrary(&spawnerAnims);
    sp.anim.Play("idle", true);
    sp.anim.ApplyTo(sp.sprite, false);

    spawners.push_back(sp);
    world.MarkChunksInRect(x, y, sp.w, sp.h);

    return spawners.back();
}

NPCGoal& NPCSystem::AddGoal(WorldSim& world, int x, int y)
{
    NPCGoal goal{};
    goal.x = x;
    goal.y = y;
    goal.w = 12;
    goal.h = 12;
    goal.capturedCount = 0;

    goal.sprite.tex = &structuresTex;
    goal.anim.SetLibrary(&goalAnims);
    goal.anim.Play("idle", true);
    goal.anim.ApplyTo(goal.sprite, false);

    goals.push_back(goal);
    world.MarkChunksInRect(x, y, goal.w, goal.h);

    return goals.back();
}

NPCBonus& NPCSystem::AddBonus(WorldSim& world, int x, int y)
{
    NPCBonus bonus{};
    bonus.x = x;
    bonus.y = y;
    bonus.w = 12;
    bonus.h = 12;
    bonus.claimed = false;
    bonus.sprite.tex = &structuresTex;
    bonus.anim.SetLibrary(&bonusAnims);
    bonus.anim.Play("idle", true);
    bonus.anim.ApplyTo(bonus.sprite);

    bonuses.push_back(bonus);
    world.MarkChunksInRect(x, y, bonus.w, bonus.h);

    return bonuses.back();
}

bool NPCSystem::EraseEntitiesInCircle(WorldSim& world, int cx, int cy, int radius, bool eraseNPCs, bool eraseSpawners, bool eraseGoals, bool eraseBonuses)
{
    bool removedAny = false;

    auto eraseNPCRange = [&](auto& vec, auto&& pred) {
        const auto oldSize = vec.size();
        vec.erase(std::remove_if(vec.begin(), vec.end(), pred), vec.end());
        if (vec.size() != oldSize) removedAny = true;
    };

    if (eraseNPCs) {
        eraseNPCRange(npcs, [&](const NPC& n) {
            return CircleIntersectsRect(cx, cy, radius, n.x, n.y, n.w, n.h);
        });
    }

    if (eraseSpawners) {
        std::vector<int> removedIds;
        for (int i = 0; i < (int)spawners.size(); ++i) {
            const NPCSpawner& sp = spawners[i];
            if (CircleIntersectsRect(cx, cy, radius, sp.x, sp.y, sp.w, sp.h)) {
                removedIds.push_back(i);
            }
        }

        if (!removedIds.empty()) {
            eraseNPCRange(spawners, [&](const NPCSpawner& sp) {
                return CircleIntersectsRect(cx, cy, radius, sp.x, sp.y, sp.w, sp.h);
            });

            for (NPC& n : npcs) {
                if (n.spawnerId < 0) continue;

                int shift = 0;
                bool removedSpawner = false;
                for (int id : removedIds) {
                    if (n.spawnerId == id) { removedSpawner = true; break; }
                    if (id < n.spawnerId) ++shift;
                }

                if (removedSpawner) n.spawnerId = -1;
                else n.spawnerId -= shift;
            }
        }
    }

    if (eraseGoals) {
        std::vector<int> removedIds;
        for (int i = 0; i < (int)goals.size(); ++i) {
            const NPCGoal& g = goals[i];
            if (CircleIntersectsRect(cx, cy, radius, g.x, g.y, g.w, g.h)) {
                removedIds.push_back(i);
            }
        }

        if (!removedIds.empty()) {
            eraseNPCRange(goals, [&](const NPCGoal& g) {
                return CircleIntersectsRect(cx, cy, radius, g.x, g.y, g.w, g.h);
            });

            for (NPC& n : npcs) {
                if (n.goalId < 0) continue;

                int shift = 0;
                bool removedGoal = false; 
                for (int id : removedIds) {
                    if (n.goalId == id) { removedGoal = true; break; }
                    if (id < n.goalId) ++shift;
                }

                if (removedGoal) {
                    n.goalId = -1;
                    n.parked = false;
                }
                else {
                    n.goalId -= shift;
                }
            }
        }
    }

    if (eraseBonuses) {
        eraseNPCRange(bonuses, [&](const NPCBonus& b) {
            return CircleIntersectsRect(cx, cy, radius, b.x, b.y, b.w, b.h);
        });
    }

    if (removedAny) {
        RebuildOcc(world);
    }

    return removedAny;
}

bool NPCSystem::AnyBonusClaimed() const
{
    for (const NPCBonus& b : bonuses) {
        if (b.claimed) return true;
    }
    return false;
}

int NPCSystem::TotalCapturedCount() const
{
    int total = 0;
    for (const NPCGoal& g : goals) total += std::fmax(0, g.capturedCount);
    return total;
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
        if (n.parked) continue;

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
            if (!IsNPCPassableMat(world.BackMatAtIndex(i))) return false;
            const int occId = occ.empty() ? 0 : occ[i];
            if (occId != 0 && occId != ignoreId) return false;
        }
    return true;
}

bool NPCSystem::CheckNPCDie(const WorldSim& world, int x, int y, int w, int h) const
{
    auto isLava = [&](int gx, int gy) -> bool {
        if (!world.InRange(gx, gy)) return false;
        return IsHazardMat(world.BackMatAtIndex(world.LinearIndex(gx, gy)));
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

bool NPCSystem::IsInWater(const WorldSim& world, int x, int y, int w, int h) const
{
    for (int yy = 0; yy < h-5; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            const int gx = x + xx;
            const int gy = y + yy;
            if (!world.InRange(gx, gy)) continue;
            if (world.BackMatAtIndex(world.LinearIndex(gx, gy)) == (uint8)Material::Water) {
                return true;
            }
        }
    }
    return false;
}

bool NPCSystem::IsInVine(const WorldSim& world, int x, int y, int w, int h) const
{
    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            const int gx = x + xx; 
            const int gy = y + yy;
            if (!world.InRange(gx, gy)) continue;
            if (world.BackMatAtIndex(world.LinearIndex(gx, gy)) == (uint8)Material::Vine) {
                return true;
            }
        }
    }
    return false;
}

bool NPCSystem::HasVineBelow(const WorldSim& world, int x, int y, int w, int h) const
{
    const int gy = y + h;
    for (int xx = 0; xx < w; ++xx) {
        const int gx = x + xx;
        if (!world.InRange(gx, gy)) continue;
        if (world.BackMatAtIndex(world.LinearIndex(gx, gy)) == Material::Vine) {
            return true;
        }
    }
    return false;
}

bool NPCSystem::IsBuriedInSand(const WorldSim& world, int x, int y, int w, int h, int ignoreId) const
{
    int topSand = 0;
    for (int xx = 0; xx < w; ++xx) {
        const int gx = x + xx;
        const int gy = y - 1;
        if (!world.InRange(gx, gy)) continue;
        if (world.BackMatAtIndex(world.LinearIndex(gx, gy)) == (uint8)Material::Sand) {
            ++topSand;
        }
    }

    if (topSand <= 0) return false;

    const bool canRise = RectFreeOnBack(world, x, y - 1, w, h, ignoreId);
    const bool canMoveLeft = RectFreeOnBack(world, x - 1, y, w, h, ignoreId);
    const bool canMoveRight = RectFreeOnBack(world, x + 1, y, w, h, ignoreId);

    return !canRise && !canMoveLeft && !canMoveRight;
}

int NPCSystem::CountAliveFromSpawner(int spawnerId) const
{
    int count = 0;
    for (const auto& n : npcs) {
        if (!n.alive) continue;
        if (n.spawnerId == spawnerId) ++count;
    }
    return count;
}

bool NPCSystem::TrySpawnFromSpawner(WorldSim& world, int spawnerId)
{
    if (spawnerId < 0 || spawnerId >= (int)spawners.size()) return false;

    NPCSpawner& sp = spawners[spawnerId];
    NPC temp = MakeDefaultNPC(&npcTex, &npcAnims,
        sp.x + (sp.w - 12) / 2,
        sp.y + (sp.h-12)/ 2,
        1);

    const int hbX = temp.x + temp.hbOffX;
    const int hbY = temp.y + temp.hbOffY;
    if (!RectFreeOnBack(world, hbX, hbY, temp.hbW, temp.hbH, 0)) {
        return false;
    }

    NPC& n = AddNPC(world, temp.x, temp.y, temp.dir);
    n.spawnerId = spawnerId;
    return true;
}

bool NPCSystem::TryParkNPCInGoal(NPC& n)
{
    if (!n.alive || n.parked) return false;

    const int hx = n.x + n.hbOffX;
    const int hy = n.y + n.hbOffY;
    for (int i = 0; i < (int)goals.size(); ++i) {
        NPCGoal& goal = goals[i];
        if (!RectsOverlap(hx, hy, n.hbW, n.hbH, goal.x, goal.y, goal.w, goal.h)) continue;

        n.parked = true;
        n.goalId = i;
        n.anim.Play("idle", true);
        goal.capturedCount += 1;
        return true;
    }

    return false;
}

bool NPCSystem::TryTouchBonus(NPC& n)
{
    if (!n.alive) return false;

    const int hx = n.x + n.hbOffX;
    const int hy = n.y + n.hbOffY;
    for (NPCBonus& bonus : bonuses) {
        if (bonus.claimed) continue;
        if (!RectsOverlap(hx, hy, n.hbW, n.hbH, bonus.x, bonus.y, bonus.w, bonus.h)) continue;
        bonus.claimed = true;
        return true;
    }

    return false;
}

void NPCSystem::MoveNPCs(WorldSim& world, float fixedTimeStep)
{
    // Spawners
    for (int i = 0; i < (int)spawners.size(); ++i) {
        NPCSpawner& sp = spawners[i];
        sp.spawnAcc += fixedTimeStep;

        const int aliveCount = CountAliveFromSpawner(i);
        if (aliveCount >= sp.maxAlive) {
            if (sp.spawnAcc > sp.spawnCooldown) sp.spawnAcc = sp.spawnCooldown;
            continue;
        }

        if (sp.spawnAcc >= sp.spawnCooldown) {
            if (TrySpawnFromSpawner(world, i)) {
                sp.spawnAcc = 0.0f;
            }
            else {
                sp.spawnAcc = sp.spawnCooldown;
            }
        }
    }

    RebuildOcc(world);

    npcMoveAcc += fixedTimeStep;
    const float npcMoveInterval = 1.0f / std::fmax(0.001f, npcCellsPerSec);
    if (npcMoveAcc < npcMoveInterval) return;
    npcMoveAcc -= npcMoveInterval;

    constexpr int kMaxStep = 2;
    for (int i = 0; i < (int)npcs.size(); ++i) {
        auto& n = npcs[i];
        if (!n.alive) continue;
        if (n.anim.CurrentName() == "dieFire" || n.anim.CurrentName() == "dieNormal") continue;
        const int id = i + 1;

        auto killNPC = [&](std::string animation) {
            n.anim.Play(animation, false);
            n.parked = false;
            n.goalId = -1;
            n.speechTimer = 0.0f;
            n.speechMessage = -1;
        };

        const int hbX = n.x + n.hbOffX;
        const int hbY = n.y + n.hbOffY; 

        if (CheckNPCDie(world, hbX, hbY, n.hbW, n.hbH)) {
            killNPC("dieFire");
            continue;
        }

        if (IsInWater(world, hbX, hbY, n.hbW, n.hbH) || IsBuriedInSand(world, hbX, hbY, n.hbW, n.hbH, id)) {
            n.oxygenTime += fixedTimeStep;
            n.drowning = true;
            if (n.oxygenTime >= 1.0f) {
                killNPC("dieNormal");
                continue;
            }
        }
        else {
            n.oxygenTime = 0.0f;
            n.drowning = false;
        }

        TryTouchBonus(n);
        if (TryParkNPCInGoal(n)) {
            continue;
        }

        if (n.parked) {
            n.anim.Play("idle", false);
            continue;
        }

        //Breakable platform
        const int footY = n.y + n.hbOffY + n.hbH;
        bool broke = false;
        for (int xx = 0; xx < n.hbW; ++xx) {
            const int gx = n.x + n.hbOffX + xx;
            if (!world.InRange(gx, footY)) continue;
            const int wi = world.LinearIndex(gx, footY);
            if (world.BackMatAtIndex(wi) == (uint8)Material::FragilePlatform) {
                world.SetCell(gx, footY, (uint8)Material::Empty);
                broke = true;
            }
        }
        if (broke) {
            RebuildOcc(world);
        }

        const int nx = n.x + n.dir;

        //Vine
        if (IsInVine(world, hbX, hbY, n.hbW, n.hbH)) {
            if (RectFreeOnBack(world, hbX, hbY - 1, n.hbW, n.hbH, id)) {
                n.y -= 1;
                TryTouchBonus(n);
                if (!TryParkNPCInGoal(n)) {
                    n.anim.Play("idle", false);
                }
                continue;
            }

            if (RectFreeOnBack(world, nx + n.hbOffX, hbY, n.hbW, n.hbH, id)) {
                n.x = nx;
                TryTouchBonus(n);
                if (!TryParkNPCInGoal(n)) {
                    n.anim.Play("walk", false);
                }
                continue;
            }
        }

        //Gravity
        if (!HasVineBelow(world, hbX, hbY, n.hbW, n.hbH) &&
            RectFreeOnBack(world, hbX, hbY + 1, n.hbW, n.hbH, id)) {
            n.y += 1;
            TryTouchBonus(n);
            if (!TryParkNPCInGoal(n) &&
                RectFreeOnBack(world, hbX, hbY + 2, n.hbW, n.hbH, id)) {
                n.anim.Play("fall", false);
            }
            continue;
        }

        //Step down
        if (RectFreeOnBack(world, nx + n.hbOffX, hbY + 1, n.hbW, n.hbH, id)) {
            n.x = nx;
            n.y += 1;
            TryTouchBonus(n);
            if (!TryParkNPCInGoal(n)) {
                n.anim.Play("walk", false);
            }
            continue;
        }


        //Forward
        if (RectFreeOnBack(world, nx + n.hbOffX, hbY, n.hbW, n.hbH, id)) {
            n.x = nx;
            TryTouchBonus(n);
            if (!TryParkNPCInGoal(n)) {
                n.anim.Play("walk", false);
            }
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
            TryTouchBonus(n);
            if (!TryParkNPCInGoal(n)) {
                n.anim.Play("walk", false);
            }
            break;
        }

        if (climbed) {
            continue;
        }

        const int otherDir = -n.dir;
        const int otherNx = n.x + otherDir;

        if (RectFreeOnBack(world, otherNx + n.hbOffX, hbY, n.hbW, n.hbH, id)) {
            n.dir = otherDir;
            n.x = otherNx;
            TryTouchBonus(n);
            if (!TryParkNPCInGoal(n)) {
                n.anim.Play("walk", false);
            }
            continue;
        }

        bool climbedOther = false;
        for (int step = 1; step <= kMaxStep; ++step) {
            if (!RectFreeOnBack(world, otherNx + n.hbOffX, (n.y - step) + n.hbOffY, n.hbW, n.hbH, id)) continue;

            bool support = false;
            const int supY = (n.y - step) + n.hbOffY + n.hbH;
            for (int xx = 0; xx < n.hbW; ++xx)
                support |= solid((otherNx + n.hbOffX) + xx, supY);
            if (!support) continue;

            n.dir = otherDir;
            n.y -= step;
            n.x = otherNx;
            climbedOther = true;
            TryTouchBonus(n);
            if (!TryParkNPCInGoal(n)) {
                n.anim.Play("walk", false);
            }
            break;
        }

        if (!climbedOther) {
            n.anim.Play("idle", false);
        }
    }
}

void NPCSystem::AnimateNPCs(Engine& engine, float dt)
{
    App* app = engine.app;
    if (!app || !app->renderer) return;

    constexpr float kSpeechDuration = 3.0f;

    for (auto& n : npcs) {
        if (n.speechTimer > 0.0f) {
            n.speechTimer = std::fmax(0.0f, n.speechTimer - dt);
            if (n.speechTimer <= 0.0f) n.speechMessage = -1;
        }

        if (!n.alive || n.parked || n.anim.CurrentName() == "dieFire" || n.anim.CurrentName() == "dieNormal") continue;
        if (n.speechTimer > 0.0f) continue;

        if ((std::rand() % 1000) == 0) {
            n.speechMessage = std::rand() % 5;
            n.speechTimer = kSpeechDuration;
        }
    }

    for (auto& sp : spawners) {
        sp.anim.Update(dt);
        sp.anim.ApplyTo(sp.sprite, false);

        float sx, sy, sw, sh;
        if (!engine.WorldRectToScreen((float)sp.x, (float)sp.y, (float)sp.w, (float)sp.h,
            app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh))
            continue;

        sp.sprite.x = std::floor(sx);
        sp.sprite.y = std::floor(sy);
        sp.sprite.w = std::floor(sw);
        sp.sprite.h = std::floor(sh);
        sp.sprite.layer = RenderLayer::WORLD;
        sp.sprite.sort = -20;
        app->renderer->Queue(sp.sprite);
    }

    for (auto& bonus : bonuses) {
        if (bonus.claimed) {
            bonus.anim.Play("lighted", false);
        }
        bonus.anim.Update(dt);
        bonus.anim.ApplyTo(bonus.sprite);


        float sx, sy, sw, sh;
        if (!engine.WorldRectToScreen((float)bonus.x, (float)bonus.y, (float)bonus.w, (float)bonus.h,
            app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh))
            continue;

        bonus.sprite.x = std::floor(sx);
        bonus.sprite.y = std::floor(sy);
        bonus.sprite.w = std::floor(sw);
        bonus.sprite.h = std::floor(sh);
        bonus.sprite.layer = RenderLayer::WORLD;
        bonus.sprite.sort = -15;
        app->renderer->Queue(bonus.sprite);
    }

    for (auto& goal : goals) {
        goal.anim.Update(dt);
        goal.anim.ApplyTo(goal.sprite, false);

        float sx, sy, sw, sh;
        if (!engine.WorldRectToScreen((float)goal.x, (float)goal.y, (float)goal.w, (float)goal.h,
            app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh))
            continue;

        goal.sprite.x = std::floor(sx);
        goal.sprite.y = std::floor(sy);
        goal.sprite.w = std::floor(sw);
        goal.sprite.h = std::floor(sh);
        goal.sprite.layer = RenderLayer::WORLD;
        goal.sprite.sort = -10;
        app->renderer->Queue(goal.sprite);
    }

    for (auto& n : npcs) {
        n.anim.Update(dt);
        n.anim.ApplyTo(n.sprite, n.dir < 0);

        if ((n.anim.CurrentName() == "dieFire" || n.anim.CurrentName() == "dieNormal") && n.anim.IsFinished()) {
            n.alive = false;
            continue;
        }
        if (!n.alive) continue;

        float sx, sy, sw, sh;
        if (!engine.WorldRectToScreen((float)n.x, (float)n.y, (float)n.w, (float)n.h,
            app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh))
            continue;

        n.sprite.x = std::floor(sx);
        n.sprite.y = std::floor(sy);
        n.sprite.w = std::floor(sw);
        n.sprite.h = std::floor(sh);
        n.sprite.layer = RenderLayer::WORLD;
        n.sprite.sort = 0;
        app->renderer->Queue(n.sprite);
    }
}
