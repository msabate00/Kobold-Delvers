#pragma once

#include "level.h"

struct App;
struct Engine;
class WorldSim;
class NPCSystem;

class LevelIO
{
public:
    void ExportLevel(const WorldSim& world, const NPCSystem& npcs, Level& out) const;
    bool ImportLevel(Engine& engine, WorldSim& world, NPCSystem& npcs, const Level& in);

    bool SaveLevel(const WorldSim& world, const NPCSystem& npcs, const char* path) const;
    bool LoadLevel(Engine& engine, WorldSim& world, NPCSystem& npcs, const char* path);
};
