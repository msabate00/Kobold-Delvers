#pragma once
#include <cstdint>
#include "app/defs.h"
#include "material.h"
#include "audio/audio.h"

struct Engine;
class WorldSim;
class NPCSystem;

class PaintTool
{
public:
    void Clear();

    void Paint(Engine& engine, WorldSim& world, NPCSystem& npcs, int screenX, int screenY, Material m, int radius, bool shift);
    void StopPaint(Engine& engine);

private:
    enum class PaintAxis : uint8 { None, X, Y };

    bool paintActive = false;
    bool paintShiftPrev = false;
    PaintAxis paintAxis = PaintAxis::None;

    Vec2<int> paintAnchor{ 0,0 };
    Vec2<int> paintLast{ 0,0 };

    AudioInstance paintInstance{};

    bool npcDrawed = false;
};
