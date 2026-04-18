#pragma once

#include "level.h"

struct App;
struct Engine;
class WorldSim;
class NPCSystem;
class PlayerSystem;

class LevelIO
{
public:
    void ExportLevel(const WorldSim& world, const NPCSystem& npcs, const PlayerSystem& players, Level& out) const;
    bool ImportLevel(Engine& engine, WorldSim& world, NPCSystem& npcs, PlayerSystem& players, const Level& in);

    bool SaveLevel(const WorldSim& world, const NPCSystem& npcs, const PlayerSystem& players, const char* path) const;
    bool LoadLevel(Engine& engine, WorldSim& world, NPCSystem& npcs, PlayerSystem& players, const char* path);
};
