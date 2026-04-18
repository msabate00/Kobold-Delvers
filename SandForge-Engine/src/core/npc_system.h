#pragma once

#include <vector>

#include "render/sprite.h"
#include "render/texture.h"
#include "render/anim.h"

struct Engine;
class WorldSim;

struct NPC {
    int x = 0, y = 0;
    int w = 2, h = 4;

    // Hitbox
    int hbOffX = 0, hbOffY = 0;
    int hbW = 2, hbH = 4;

    int dir = 1;
    bool alive = true;
    bool parked = false;

    float oxygenTime = 0.0f;
    float speechTimer = 0.0f;
    int speechMessage = -1;
    bool drowning = false;

    int spawnerId = -1;
    int goalId = -1;

    Sprite sprite;
    SpriteAnimPlayer anim;
};

struct NPCSpawner {
    int x = 0, y = 0;
    int w = 12, h = 12;

    int maxAlive = 5;
    float spawnCooldown = 0.5f;
    float spawnAcc = 0.0f;

    Sprite sprite;
    SpriteAnimPlayer anim;
};

struct NPCGoal {
    int x = 0, y = 0;
    int w = 12, h = 12;

    int capturedCount = 0;

    Sprite sprite;
    SpriteAnimPlayer anim;
};

struct NPCBonus {
    int x = 0, y = 0;
    int w = 12, h = 12;

    bool claimed = false;

    Sprite sprite;
    SpriteAnimPlayer anim;
};

class NPCSystem
{
public:
    bool Awake(const WorldSim& world);
    bool Start();

    void Clear(const WorldSim& world);

    NPC& AddNPC(WorldSim& world, int x, int y, int dir = 1);
    NPCSpawner& AddSpawner(WorldSim& world, int x, int y);
    NPCGoal& AddGoal(WorldSim& world, int x, int y);
    NPCBonus& AddBonus(WorldSim& world, int x, int y);

    bool EraseEntitiesInCircle(WorldSim& world, int cx, int cy, int radius,
        bool eraseNPCs = true, bool eraseSpawners = true, bool eraseGoals = true, bool eraseBonuses = true);

    const std::vector<NPC>& GetNPCs() const { return npcs; }
    const std::vector<NPCSpawner>& GetSpawners() const { return spawners; }
    const std::vector<NPCGoal>& GetGoals() const { return goals; }
    const std::vector<NPCBonus>& GetBonuses() const { return bonuses; }

    const std::vector<int>& Occ() const { return occ; }

    bool AnyBonusClaimed() const;
    int TotalCapturedCount() const;

    void RebuildOcc(const WorldSim& world);
    void AddOccRect(const WorldSim& world, int x, int y, int w, int h, int occId);
    void MoveNPCs(WorldSim& world, float fixedTimeStep);
    void AnimateNPCs(Engine& engine, float dt);

private:
    bool RectFreeOnBack(const WorldSim& world, int x, int y, int w, int h, int ignoreId) const;
    bool CheckNPCDie(const WorldSim& world, int x, int y, int w, int h) const;
    bool IsInWater(const WorldSim& world, int x, int y, int w, int h) const;
    bool IsInVine(const WorldSim& world, int x, int y, int w, int h) const;
    bool IsBuriedInSand(const WorldSim& world, int x, int y, int w, int h, int ignoreId) const;

    bool HasVineBelow(const WorldSim& world, int x, int y, int w, int h) const;

    bool TrySpawnFromSpawner(WorldSim& world, int spawnerId);
    bool TryParkNPCInGoal(NPC& n);
    bool TryTouchBonus(NPC& n);
    int CountAliveFromSpawner(int spawnerId) const;

private:
    std::vector<NPC> npcs;
    std::vector<NPCSpawner> spawners;
    std::vector<NPCGoal> goals;
    std::vector<NPCBonus> bonuses;
    std::vector<int> occ;

    Texture2D npcTex;
    Texture2D structuresTex;

    SpriteAnimLibrary npcAnims;
    SpriteAnimLibrary spawnerAnims;
    SpriteAnimLibrary goalAnims;
    SpriteAnimLibrary bonusAnims;

    float npcMoveAcc = 0.0f;
    float npcCellsPerSec = 32.0f;
};
