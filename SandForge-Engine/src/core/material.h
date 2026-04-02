#pragma once
#include <cstdint>
#include <array>
#include <string_view>
#include "app/defs.h"

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
    NpcBonusCell
};

struct Cell {
    uint8 m = (uint8)Material::Empty;
    uint8 fromLevel = 0;
}; 

struct MatProps {
    std::string_view name;

    RGBAI color;
    uint8 density = 1;

    void (*update)(Engine&, int x, int y, const Cell& self) = nullptr;
};

const MatProps& matProps(uint8 id);
void registerDefaultMaterials();

bool isVolatile(uint8 m);
