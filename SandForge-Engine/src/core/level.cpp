#include "level.h"

#include <cstdio>
#include <cstring>



bool Level::IsValid() const
{
    if (w <= 0 || h <= 0) return false;
    return grid.size() == (size_t)(w * h);
}

bool SaveLevelFile(const char* path, const Level& lvl)
{
    if (!path || !path[0]) return false;
    if (!lvl.IsValid()) return false;

    FILE* f = std::fopen(path, "wb");
    if (!f) return false;

    LevelHeader hdr;
    hdr.version = LVL_VERSION;
    hdr.w = (uint32)lvl.w;
    hdr.h = (uint32)lvl.h;
    hdr.npcCount = (uint32)lvl.npcs.size();

    if (std::fwrite(&hdr, sizeof(hdr), 1, f) != 1) { std::fclose(f); return false; }

    const size_t gridBytes = (size_t)(lvl.w * lvl.h);
    if (gridBytes > 0) {
        if (std::fwrite(lvl.grid.data(), 1, gridBytes, f) != gridBytes) { std::fclose(f); return false; }
    }

    for (const LevelNPC& n : lvl.npcs)
    {
        if (std::fwrite(&n.x, sizeof(n.x), 1, f) != 1) { std::fclose(f); return false; }
        if (std::fwrite(&n.y, sizeof(n.y), 1, f) != 1) { std::fclose(f); return false; }
        if (std::fwrite(&n.w, sizeof(n.w), 1, f) != 1) { std::fclose(f); return false; }
        if (std::fwrite(&n.h, sizeof(n.h), 1, f) != 1) { std::fclose(f); return false; }
        if (std::fwrite(&n.dir, sizeof(n.dir), 1, f) != 1) { std::fclose(f); return false; }

        uint8_t alive = n.alive ? 1 : 0;
        if (std::fwrite(&alive, sizeof(alive), 1, f) != 1) { std::fclose(f); return false; }
    }

    std::fclose(f);
    return true;
}

bool LoadLevelFile(const char* path, Level& lvl)
{
    if (!path || !path[0]) return false;

    FILE* f = std::fopen(path, "rb");
    if (!f) return false;

    LevelHeader hdr;
    if (std::fread(&hdr, sizeof(hdr), 1, f) != 1) { std::fclose(f); return false; }
    if (hdr.version != LVL_VERSION) { std::fclose(f); return false; }

    if (hdr.w == 0 || hdr.h == 0) { std::fclose(f); return false; }

    //Por si aca explota al guardar
    if (hdr.w > 16384 || hdr.h > 16384) { std::fclose(f); return false; }
    if (hdr.npcCount > hdr.w * hdr.h) { fclose(f); return false; }

    const size_t gridBytes = (size_t)hdr.w * (size_t)hdr.h;

    lvl.w = (int)hdr.w;
    lvl.h = (int)hdr.h;
    lvl.grid.resize(gridBytes);
    lvl.npcs.clear();
    lvl.npcs.reserve(hdr.npcCount);

    if (gridBytes > 0) {
        if (std::fread(lvl.grid.data(), 1, gridBytes, f) != gridBytes) { std::fclose(f); return false; }
    }

    for (uint32 i = 0; i < hdr.npcCount; ++i)
    {
        LevelNPC n{};

        if (std::fread(&n.x, sizeof(n.x), 1, f) != 1) { std::fclose(f); return false; }
        if (std::fread(&n.y, sizeof(n.y), 1, f) != 1) { std::fclose(f); return false; }
        if (std::fread(&n.w, sizeof(n.w), 1, f) != 1) { std::fclose(f); return false; }
        if (std::fread(&n.h, sizeof(n.h), 1, f) != 1) { std::fclose(f); return false; }
        if (std::fread(&n.dir, sizeof(n.dir), 1, f) != 1) { std::fclose(f); return false; }

        uint8_t alive = 0;
        if (std::fread(&alive, sizeof(alive), 1, f) != 1) { std::fclose(f); return false; }
        n.alive = (alive != 0);

        lvl.npcs.push_back(n);
    }

    std::fclose(f);
    return lvl.IsValid();
}
