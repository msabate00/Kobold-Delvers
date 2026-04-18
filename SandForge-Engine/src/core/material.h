#pragma once
#include <cstdint>
#include <array>
#include <string_view>
#include "app/defs.h"
#include "render/atlas.h"

struct Engine;

enum Material : uint8 {
    Null = -1,
    Empty = 0,
    Sand,
    Water,
    Stone, 
    Wood,
    Fire,
    Lava,
    Smoke, 
    Steam,
    NpcCell,
    NpcSpawnerCell,
    NpcGoalCell,
    NpcBonusCell,
    Vine,
    Snow,
    Oil,
    Coal,
    HotCoal,
    FlammableGas,
    FragilePlatform,
    Acid,
    Dynamite,
    PlayerCell,
    PlayerTriggerCell,
    COUNT
};

struct Cell {
    uint8 m = (uint8)Material::Empty;
    uint8 fromLevel = 0;
}; 

struct MatProps {
    std::string_view name;

    RGBAI color;
    uint8 density = 1;
    AtlasRectPx rect;

    void (*update)(Engine&, int x, int y, const Cell& self) = nullptr;
};

const MatProps& matProps(uint8 id);
void registerDefaultMaterials();

bool isVolatile(uint8 m);
bool IsHeatMat(uint8 m);
bool IsFlammableMat(uint8 m);
bool IsAcidTarget(uint8 m);

void ExplodeIntoFire(Engine& E, int cx, int cy, int radius);
void TryIgniteNeighbours(Engine& E, int x, int y);
