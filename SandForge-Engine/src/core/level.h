#pragma once

#include <vector>

#include "app/defs.h"

static const uint32 LVL_VERSION = 2;

struct LevelHeader {
    uint32 version;
    uint32 w;
    uint32 h;
    uint32 npcCount;
};

struct LevelNPC {
    int x = 0, y = 0;
    int w = 2, h = 4;
    int dir = 1;
    uint8 alive = true;
};

struct Level {
    int w = 0, h = 0;
    std::vector<uint8> grid;
    std::vector<LevelNPC> npcs;

    bool IsValid() const;
};

bool SaveLevelFile(const char* path, const Level& lvl);
bool LoadLevelFile(const char* path, Level& lvl);
