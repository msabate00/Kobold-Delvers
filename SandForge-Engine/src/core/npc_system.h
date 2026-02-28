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

    //Hitbox
    int hbOffX = 0, hbOffY = 0;
    int hbW = 2, hbH = 4;

    int dir = 1;
    bool alive = true;

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
    const std::vector<NPC>& GetNPCs() const { return npcs; }

    const std::vector<int>& Occ() const { return occ; }

    void RebuildOcc(const WorldSim& world);
    void MoveNPCs(WorldSim& world, float fixedTimeStep);
    void AnimateNPCs(Engine& engine, float dt);

private:
    bool RectFreeOnBack(const WorldSim& world, int x, int y, int w, int h, int ignoreId) const;
    bool CheckNPCDie(const WorldSim& world, int x, int y, int w, int h) const;

private:
    std::vector<NPC> npcs;
    std::vector<int> occ;

    Texture2D npcTex;
    SpriteAnimLibrary npcAnims;

    float npcMoveAcc = 0.0f;
    float npcCellsPerSec = 32.0f;
};
