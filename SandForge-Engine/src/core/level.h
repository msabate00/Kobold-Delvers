#pragma once

#include <vector>

#include "app/defs.h"

static const uint32 LVL_VERSION = 5;

struct LevelHeaderV2 {
    uint32 version;
    uint32 w;
    uint32 h;
    uint32 npcCount;
};

struct LevelHeaderV3 {
    uint32 version;
    uint32 w;
    uint32 h;
    uint32 npcCount;
    uint32 spawnerCount;
    uint32 goalCount;
};

struct LevelHeaderV4 {
    uint32 version;
    uint32 w;
    uint32 h;
    uint32 npcCount;
    uint32 spawnerCount;
    uint32 goalCount;
    uint32 bonusCount;
    int materialBudgetMax;
    int materialBudgetStar;
};

struct LevelHeader {
    uint32 version;
    uint32 w;
    uint32 h;
    uint32 npcCount;
    uint32 spawnerCount;
    uint32 goalCount;
    uint32 bonusCount;
    uint32 playerCount;
    uint32 playerTriggerCount;
    int materialBudgetMax;
    int materialBudgetStar;
};

struct LevelNPC {
    int x = 0, y = 0;
    int w = 2, h = 4;
    int dir = 1;
    int spawnerId = -1;
    int goalId = -1;
    uint8 alive = true;
    uint8 parked = false;
};

struct LevelSpawner {
    int x = 0, y = 0;
    int w = 12, h = 12;
    int maxAlive = 5;
};

struct LevelGoal {
    int x = 0, y = 0;
    int w = 12, h = 12;
    int capturedCount = 0;
};

struct LevelBonus {
    int x = 0, y = 0;
    int w = 12, h = 12;
    uint8 claimed = false;
};


struct LevelPlayer {
    int x = 0, y = 0;
    int w = 12, h = 12;
    int dir = 1;
    int heldMaterial = (int)0;
    uint8 alive = true;
};

struct LevelPlayerTrigger {
    int x = 0, y = 0;
    int w = 12, h = 12;
    int material = (int)1;
};

struct LevelRules {
    int materialBudgetMax = 0;
    int materialBudgetStar = 0;
};

struct Level {
    int w = 0, h = 0;
    std::vector<uint8> grid;
    std::vector<LevelNPC> npcs;
    std::vector<LevelSpawner> spawners;
    std::vector<LevelGoal> goals;
    std::vector<LevelBonus> bonuses;
    bool hasPlayer = false;
    LevelPlayer player;
    std::vector<LevelPlayerTrigger> playerTriggers;
    LevelRules rules;

    bool IsValid() const;
};

bool SaveLevelFile(const char* path, const Level& lvl);
bool LoadLevelFile(const char* path, Level& lvl);
