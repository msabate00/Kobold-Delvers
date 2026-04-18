#pragma once

#include <vector>

#include "render/sprite.h"
#include "render/texture.h"
#include "render/anim.h"
#include "core/material.h"

struct Engine;
class WorldSim;

struct Player {
    int x = 0;
    int y = 0;
    int w = 12;
    int h = 12;

    int hbOffX = 0;
    int hbOffY = 0;
    int hbW = 6;
    int hbH = 7;

    int dir = 1;
    bool alive = true;

    float oxygenTime = 0.0f;
    bool drowning = false;

    int jumpCellsLeft = 0;
    Material heldMaterial = Material::Empty;
    float placeCooldown = 0.0f;
    float jumpBuffer = 0.0f;
    float coyoteTime = 0.0f;

    Sprite sprite;
    SpriteAnimPlayer anim;
};

struct PlayerMaterialTrigger {
    int x = 0;
    int y = 0;
    int w = 12;
    int h = 12;

    Material material = Material::Sand;

    Sprite sprite;
};

class PlayerSystem
{
public:
    bool Awake(const WorldSim& world);
    bool Start();

    void Clear(const WorldSim& world);

    Player& AddPlayer(WorldSim& world, int x, int y, Material startMaterial = Material::Empty);
    PlayerMaterialTrigger& AddPlayerMaterialTrigger(WorldSim& world, int x, int y, Material material);

    bool EraseEntitiesInCircle(WorldSim& world, int cx, int cy, int radius,
        bool erasePlayer = true, bool eraseTriggers = true);

    bool HasPlayer() const { return hasPlayer; }
    const Player* GetPlayer() const { return hasPlayer ? &player : nullptr; }
    const std::vector<PlayerMaterialTrigger>& GetPlayerMaterialTriggers() const { return playerTriggers; }

    void Move(WorldSim& world, const std::vector<int>& occ, float fixedTimeStep);
    void Animate(Engine& engine, float dt);

    bool PlayerDeathFinished() const { return deathFinished; }
    bool GetPlaceTargetCell(int mouseWorldX, int mouseWorldY, int& outX, int& outY) const;

private:
    bool RectFreeOnBack(const WorldSim& world, const std::vector<int>& occ,
        int x, int y, int w, int h, int ignoreId) const;
    bool CheckPlayerDie(const WorldSim& world, int x, int y, int w, int h) const;
    bool IsInWater(const WorldSim& world, int x, int y, int w, int h) const;
    bool IsBuriedInSand(const WorldSim& world, const std::vector<int>& occ, int x, int y, int w, int h, int ignoreId) const;
    bool IsInVine(const WorldSim& world, int x, int y, int w, int h) const;
    bool HasVineBelow(const WorldSim& world, int x, int y, int w, int h) const;
    bool TryTouchMaterialTrigger(Player& p);
    bool TryPlaceMaterial(WorldSim& world, const std::vector<int>& occ, Player& p);

private:
    Player player;
    bool hasPlayer = false;
    std::vector<PlayerMaterialTrigger> playerTriggers;

    Texture2D playerTex;
    Texture2D triggerIconsTex;
    SpriteAnimLibrary playerAnims;

    bool deathFinished = false;
    float moveAcc = 0.0f;
    float cellsPerSec = 32.0f;
};
