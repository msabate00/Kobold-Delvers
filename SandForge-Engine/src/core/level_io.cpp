#include "level_io.h"

#include "engine.h"
#include "worldsim.h"
#include "npc_system.h"
#include "app/app.h"
#include "app/log.h"

#include <algorithm>
#include <cstring>

void LevelIO::ExportLevel(const WorldSim& world, const NPCSystem& npcs, Level& out) const
{
    out.w = world.GridW();
    out.h = world.GridH();

    const size_t n = (size_t)out.w * (size_t)out.h;
    out.grid.resize(n);
    if (n) {
        std::memcpy(out.grid.data(), world.FrontPlane(), n);
    }

    out.npcs.clear();
    out.npcs.reserve(npcs.GetNPCs().size());
    for (const auto& n : npcs.GetNPCs()) {
        LevelNPC ln{};
        ln.x = n.x;
        ln.y = n.y;
        ln.w = n.w;
        ln.h = n.h;
        ln.dir = n.dir;
        ln.spawnerId = n.spawnerId;
        ln.goalId = n.goalId;
        ln.alive = (uint8)n.alive;
        ln.parked = (uint8)n.parked;
        out.npcs.push_back(ln);
    }

    out.spawners.clear();
    out.spawners.reserve(npcs.GetSpawners().size());
    for (const auto& sp : npcs.GetSpawners()) {
        LevelSpawner ls{};
        ls.x = sp.x;
        ls.y = sp.y;
        ls.w = sp.w;
        ls.h = sp.h;
        ls.maxAlive = sp.maxAlive;
        out.spawners.push_back(ls);
    }

    out.goals.clear();
    out.goals.reserve(npcs.GetGoals().size());
    for (const auto& g : npcs.GetGoals()) {
        LevelGoal lg{};
        lg.x = g.x;
        lg.y = g.y;
        lg.w = g.w;
        lg.h = g.h;
        lg.capturedCount = g.capturedCount;
        out.goals.push_back(lg);
    }
}

bool LevelIO::ImportLevel(Engine& engine, WorldSim& world, NPCSystem& npcs, const Level& in)
{
    if (!in.IsValid()) {
        LOG("ERROR: ImportLevel failed (invalid level)");
        return false;
    }

    if (in.w != world.GridW() || in.h != world.GridH()) {
        engine.ResizeGrid(in.w, in.h, false);
    }

    // materiales
    const size_t n = (size_t)world.GridW() * (size_t)world.GridH();
    for (size_t i = 0; i < n; ++i) {
        const uint8 m = in.grid[i];
        const int x = (int)(i % (size_t)world.GridW());
        const int y = (int)(i / (size_t)world.GridW());
        world.SetFrontCell(x, y, m);
    }
    world.CopyFrontToBack();

    // entidades
    npcs.Clear(world);

    for (const auto& ls : in.spawners) {
        NPCSpawner& sp = npcs.AddSpawner(world, ls.x, ls.y);
        sp.w = ls.w;
        sp.h = ls.h;
        sp.maxAlive = std::fmax(1, ls.maxAlive);
    }

    for (const auto& lg : in.goals) {
        NPCGoal& g = npcs.AddGoal(world, lg.x, lg.y);
        g.w = lg.w;
        g.h = lg.h;
        g.capturedCount = std::fmax(0, lg.capturedCount);
    }

    for (const auto& ln : in.npcs) {
        NPC& n = npcs.AddNPC(world, ln.x, ln.y, ln.dir);
        n.w = ln.w;
        n.h = ln.h;
        n.alive = (ln.alive != 0);
        n.parked = (ln.parked != 0);
        n.spawnerId = ln.spawnerId;
        n.goalId = ln.goalId;
        if (n.parked) {
            n.anim.Play("idle", true);
        }
    }

    world.ResetDirtyAll();

    world.StepAccumulator() = 0.0f;
    engine.parity = 0;
    engine.stepOnce = false;

    npcs.RebuildOcc(world);

    if (engine.app) {
        engine.app->SetCameraRect(engine.app->camera.pos.x, engine.app->camera.pos.y,
            engine.app->camera.size.x, engine.app->camera.size.y);
    }

    return true;
}

bool LevelIO::SaveLevel(const WorldSim& world, const NPCSystem& npcs, const char* path) const
{
    if (!path || !path[0]) {
        LOG("ERROR: SaveLevel called with empty path");
        return false;
    }
    Level lvl;
    ExportLevel(world, npcs, lvl);
    const bool ok = SaveLevelFile(path, lvl);
    if (!ok) LOG("ERROR: SaveLevelFile failed for '%s'", path);
    else LOG("Saved level '%s' (%dx%d, npcs=%zu, spawners=%zu, goals=%zu)", path, lvl.w, lvl.h, lvl.npcs.size(), lvl.spawners.size(), lvl.goals.size());
    return ok;
}

bool LevelIO::LoadLevel(Engine& engine, WorldSim& world, NPCSystem& npcs, const char* path)
{
    if (!path || !path[0]) {
        LOG("ERROR: LoadLevel called with empty path");
        return false;
    }
    Level lvl;
    if (!LoadLevelFile(path, lvl)) {
        LOG("ERROR: LoadLevelFile failed for '%s'", path);
        return false;
    }
    const bool ok = ImportLevel(engine, world, npcs, lvl);
    if (!ok) LOG("ERROR: ImportLevel failed for '%s'", path);
    else LOG("Loaded level '%s' (%dx%d, npcs=%zu, spawners=%zu, goals=%zu)", path, lvl.w, lvl.h, lvl.npcs.size(), lvl.spawners.size(), lvl.goals.size());

    app->ResetCamera();

    return ok;
}
