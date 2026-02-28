#include "level_io.h"

#include "engine.h"
#include "worldsim.h"
#include "npc_system.h"
#include "app/app.h"

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
        ln.alive = (uint8)n.alive;
        out.npcs.push_back(ln);
    }
}

bool LevelIO::ImportLevel(Engine& engine, WorldSim& world, NPCSystem& npcs, const Level& in)
{
    if (!in.IsValid()) return false;

    //mundo
    if (in.w != world.GridW() || in.h != world.GridH()) {
        if (engine.app) {
            engine.app->gridSize = { in.w, in.h };
        }
        world.Resize(engine.app, in.w, in.h);
        npcs.Awake(world);
    }

    //materiales
    const size_t n = (size_t)world.GridW() * (size_t)world.GridH();
    for (size_t i = 0; i < n; ++i) {
        const uint8 m = in.grid[i];
        const int x = (int)(i % (size_t)world.GridW());
        const int y = (int)(i / (size_t)world.GridW());
        world.SetFrontCell(x, y, m);
    }
    world.CopyFrontToBack();

    //npcs
    npcs.Clear(world);
    for (const auto& ln : in.npcs) {
        if (!ln.alive) continue;
        NPC& n = npcs.AddNPC(world, ln.x, ln.y, ln.dir);
        n.alive = (ln.alive != 0);
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
    if (!path || !path[0]) return false;
    Level lvl;
    ExportLevel(world, npcs, lvl);
    return SaveLevelFile(path, lvl);
}

bool LevelIO::LoadLevel(Engine& engine, WorldSim& world, NPCSystem& npcs, const char* path)
{
    if (!path || !path[0]) return false;
    Level lvl;
    if (!LoadLevelFile(path, lvl)) return false;
    return ImportLevel(engine, world, npcs, lvl);
}
