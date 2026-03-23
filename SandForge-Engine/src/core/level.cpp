#include "level.h"

#include "app/log.h"

#include <cstdio>
#include <cstring>

bool Level::IsValid() const
{
    if (w <= 0 || h <= 0) return false;
    return grid.size() == (size_t)(w * h);
}

bool SaveLevelFile(const char* path, const Level& lvl)
{
    if (!path || !path[0]) {
        LOG("ERROR: SaveLevelFile empty path");
        return false;
    }
    if (!lvl.IsValid()) {
        LOG("ERROR: SaveLevelFile invalid level");
        return false;
    }

    FILE* f = std::fopen(path, "wb");
    if (!f) {
        LOG("ERROR: SaveLevelFile fopen failed for '%s'", path);
        return false;
    }

    LevelHeader hdr{};
    hdr.version = LVL_VERSION;
    hdr.w = (uint32)lvl.w;
    hdr.h = (uint32)lvl.h;
    hdr.npcCount = (uint32)lvl.npcs.size();
    hdr.spawnerCount = (uint32)lvl.spawners.size();
    hdr.goalCount = (uint32)lvl.goals.size();

    if (std::fwrite(&hdr, sizeof(hdr), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write header failed '%s'", path); return false; }

    const size_t gridBytes = (size_t)(lvl.w * lvl.h);
    if (gridBytes > 0) {
        if (std::fwrite(lvl.grid.data(), 1, gridBytes, f) != gridBytes) { std::fclose(f); LOG("ERROR: SaveLevelFile write grid failed '%s'", path); return false; }
    }

    for (const LevelNPC& n : lvl.npcs)
    {
        if (std::fwrite(&n.x, sizeof(n.x), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.x failed '%s'", path); return false; }
        if (std::fwrite(&n.y, sizeof(n.y), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.y failed '%s'", path); return false; }
        if (std::fwrite(&n.w, sizeof(n.w), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.w failed '%s'", path); return false; }
        if (std::fwrite(&n.h, sizeof(n.h), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.h failed '%s'", path); return false; }
        if (std::fwrite(&n.dir, sizeof(n.dir), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.dir failed '%s'", path); return false; }
        if (std::fwrite(&n.spawnerId, sizeof(n.spawnerId), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.spawnerId failed '%s'", path); return false; }
        if (std::fwrite(&n.goalId, sizeof(n.goalId), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.goalId failed '%s'", path); return false; }

        uint8_t alive = n.alive ? 1 : 0;
        uint8_t parked = n.parked ? 1 : 0;
        if (std::fwrite(&alive, sizeof(alive), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.alive failed '%s'", path); return false; }
        if (std::fwrite(&parked, sizeof(parked), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write npc.parked failed '%s'", path); return false; }
    }

    for (const LevelSpawner& sp : lvl.spawners)
    {
        if (std::fwrite(&sp.x, sizeof(sp.x), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write spawner.x failed '%s'", path); return false; }
        if (std::fwrite(&sp.y, sizeof(sp.y), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write spawner.y failed '%s'", path); return false; }
        if (std::fwrite(&sp.w, sizeof(sp.w), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write spawner.w failed '%s'", path); return false; }
        if (std::fwrite(&sp.h, sizeof(sp.h), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write spawner.h failed '%s'", path); return false; }
        if (std::fwrite(&sp.maxAlive, sizeof(sp.maxAlive), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write spawner.maxAlive failed '%s'", path); return false; }
    }

    for (const LevelGoal& g : lvl.goals)
    {
        if (std::fwrite(&g.x, sizeof(g.x), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write goal.x failed '%s'", path); return false; }
        if (std::fwrite(&g.y, sizeof(g.y), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write goal.y failed '%s'", path); return false; }
        if (std::fwrite(&g.w, sizeof(g.w), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write goal.w failed '%s'", path); return false; }
        if (std::fwrite(&g.h, sizeof(g.h), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write goal.h failed '%s'", path); return false; }
        if (std::fwrite(&g.capturedCount, sizeof(g.capturedCount), 1, f) != 1) { std::fclose(f); LOG("ERROR: SaveLevelFile write goal.capturedCount failed '%s'", path); return false; }
    }

    std::fclose(f);
    return true;
}

bool LoadLevelFile(const char* path, Level& lvl)
{
    if (!path || !path[0]) {
        LOG("ERROR: LoadLevelFile empty path");
        return false;
    }

    FILE* f = std::fopen(path, "rb");
    if (!f) {
        LOG("ERROR: LoadLevelFile fopen failed for '%s'", path);
        return false;
    }

    uint32 version = 0;
    if (std::fread(&version, sizeof(version), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read version failed '%s'", path); return false; }

    LevelHeader hdr{};
    hdr.version = version;
    if (version == 2) {
        LevelHeaderV2 hdr2{};
        hdr2.version = version;
        if (std::fread(&hdr2.w, sizeof(LevelHeaderV2) - sizeof(uint32), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read v2 header failed '%s'", path); return false; }
        hdr.w = hdr2.w;
        hdr.h = hdr2.h;
        hdr.npcCount = hdr2.npcCount;
        hdr.spawnerCount = 0;
        hdr.goalCount = 0;
    }
    else if (version == LVL_VERSION) {
        if (std::fread(&hdr.w, sizeof(LevelHeader) - sizeof(uint32), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read header failed '%s'", path); return false; }
    }
    else {
        std::fclose(f);
        LOG("ERROR: LoadLevelFile version mismatch '%s' (file=%u expected=%u or 2)", path, version, (unsigned)LVL_VERSION);
        return false;
    }

    if (hdr.w == 0 || hdr.h == 0) { std::fclose(f); LOG("ERROR: LoadLevelFile invalid size '%s' (%u,%u)", path, hdr.w, hdr.h); return false; }

    if (hdr.w > 16384 || hdr.h > 16384) { std::fclose(f); LOG("ERROR: LoadLevelFile size too large '%s' (%u,%u)", path, hdr.w, hdr.h); return false; }
    if (hdr.npcCount > hdr.w * hdr.h) { std::fclose(f); LOG("ERROR: LoadLevelFile npcCount invalid '%s' (npcCount=%u)", path, hdr.npcCount); return false; }
    if (hdr.spawnerCount > hdr.w * hdr.h) { std::fclose(f); LOG("ERROR: LoadLevelFile spawnerCount invalid '%s' (spawnerCount=%u)", path, hdr.spawnerCount); return false; }
    if (hdr.goalCount > hdr.w * hdr.h) { std::fclose(f); LOG("ERROR: LoadLevelFile goalCount invalid '%s' (goalCount=%u)", path, hdr.goalCount); return false; }

    const size_t gridBytes = (size_t)hdr.w * (size_t)hdr.h;

    lvl.w = (int)hdr.w;
    lvl.h = (int)hdr.h;
    lvl.grid.resize(gridBytes);
    lvl.npcs.clear();
    lvl.npcs.reserve(hdr.npcCount);
    lvl.spawners.clear();
    lvl.spawners.reserve(hdr.spawnerCount);
    lvl.goals.clear();
    lvl.goals.reserve(hdr.goalCount);

    if (gridBytes > 0) {
        if (std::fread(lvl.grid.data(), 1, gridBytes, f) != gridBytes) { std::fclose(f); LOG("ERROR: LoadLevelFile read grid failed '%s'", path); return false; }
    }

    for (uint32 i = 0; i < hdr.npcCount; ++i)
    {
        LevelNPC n{};

        if (std::fread(&n.x, sizeof(n.x), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.x failed '%s'", path); return false; }
        if (std::fread(&n.y, sizeof(n.y), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.y failed '%s'", path); return false; }
        if (std::fread(&n.w, sizeof(n.w), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.w failed '%s'", path); return false; }
        if (std::fread(&n.h, sizeof(n.h), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.h failed '%s'", path); return false; }
        if (std::fread(&n.dir, sizeof(n.dir), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.dir failed '%s'", path); return false; }

        if (version >= 3) {
            if (std::fread(&n.spawnerId, sizeof(n.spawnerId), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.spawnerId failed '%s'", path); return false; }
            if (std::fread(&n.goalId, sizeof(n.goalId), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.goalId failed '%s'", path); return false; }
        }
        else {
            n.spawnerId = -1;
            n.goalId = -1;
        }

        uint8_t alive = 0;
        if (std::fread(&alive, sizeof(alive), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.alive failed '%s'", path); return false; }
        n.alive = (alive != 0);

        if (version >= 3) {
            uint8_t parked = 0;
            if (std::fread(&parked, sizeof(parked), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read npc.parked failed '%s'", path); return false; }
            n.parked = (parked != 0);
        }
        else {
            n.parked = false;
        }

        lvl.npcs.push_back(n);
    }

    if (version >= 3) {
        for (uint32 i = 0; i < hdr.spawnerCount; ++i)
        {
            LevelSpawner sp{};
            if (std::fread(&sp.x, sizeof(sp.x), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read spawner.x failed '%s'", path); return false; }
            if (std::fread(&sp.y, sizeof(sp.y), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read spawner.y failed '%s'", path); return false; }
            if (std::fread(&sp.w, sizeof(sp.w), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read spawner.w failed '%s'", path); return false; }
            if (std::fread(&sp.h, sizeof(sp.h), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read spawner.h failed '%s'", path); return false; }
            if (std::fread(&sp.maxAlive, sizeof(sp.maxAlive), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read spawner.maxAlive failed '%s'", path); return false; }
            lvl.spawners.push_back(sp);
        }

        for (uint32 i = 0; i < hdr.goalCount; ++i)
        {
            LevelGoal g{};
            if (std::fread(&g.x, sizeof(g.x), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read goal.x failed '%s'", path); return false; }
            if (std::fread(&g.y, sizeof(g.y), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read goal.y failed '%s'", path); return false; }
            if (std::fread(&g.w, sizeof(g.w), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read goal.w failed '%s'", path); return false; }
            if (std::fread(&g.h, sizeof(g.h), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read goal.h failed '%s'", path); return false; }
            if (std::fread(&g.capturedCount, sizeof(g.capturedCount), 1, f) != 1) { std::fclose(f); LOG("ERROR: LoadLevelFile read goal.capturedCount failed '%s'", path); return false; }
            lvl.goals.push_back(g);
        }
    }

    std::fclose(f);
    return lvl.IsValid();
}
