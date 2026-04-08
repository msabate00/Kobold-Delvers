#include "paint_tool.h"
#include "engine.h"
#include "worldsim.h"
#include "npc_system.h"
#include "app/app.h"
#include <algorithm>
#include <cmath>

void PaintTool::Clear()
{
    paintActive = false;
    paintShiftPrev = false;
    paintAxis = PaintAxis::None;
    npcDrawed = false;
    paintInstance = {};
}

void PaintTool::Paint(Engine& engine, WorldSim& world, NPCSystem& npcs,
    int screenX, int screenY, Material m, int r)
{
    int cx = 0, cy = 0;
    if (!engine.ScreenToWorldCell(screenX, screenY, cx, cy)) return;

    if (!paintActive) {
        paintActive = true;
        paintAnchor = { cx, cy };
        paintLast = { cx, cy };
        paintAxis = PaintAxis::None;
        paintShiftPrev = app->shiftPressed;
        npcDrawed = false;
    }

    if (app->shiftPressed && !paintShiftPrev) {
        paintAnchor = paintLast;
        paintAxis = PaintAxis::None;
    }

    int tx = cx, ty = cy;
    if (app->shiftPressed) {
        const int dx = tx - paintAnchor.x;
        const int dy = ty - paintAnchor.y;

        const int adx = std::abs(dx);
        const int ady = std::abs(dy);

        auto pickAxis = [&]() {
            if (adx == 0 && ady == 0) return;
            if (adx > ady) paintAxis = PaintAxis::X;
            else if (ady > adx) paintAxis = PaintAxis::Y;
        };

        if (paintAxis == PaintAxis::None) pickAxis();

        if (paintAxis == PaintAxis::X) ty = paintAnchor.y;
        else if (paintAxis == PaintAxis::Y) tx = paintAnchor.x;
    }
    else {
        paintAxis = PaintAxis::None;
    }

    // NPC manual
    if (m == Material::NpcCell) {
        if (!npcDrawed) {
            npcs.AddNPC(world, tx - 5, ty - 9);
            npcDrawed = true;
        }
        paintLast = { tx, ty };
        paintShiftPrev = app->shiftPressed;
        return;
    }

    // Spawner
    if (m == Material::NpcSpawnerCell) {
        if (!npcDrawed) {
            npcs.AddSpawner(world, tx - 6, ty - 6);
            npcDrawed = true;
        }
        paintLast = { tx, ty };
        paintShiftPrev = app->shiftPressed;
        return;
    }

    // Goal
    if (m == Material::NpcGoalCell) {
        if (!npcDrawed) {
            npcs.AddGoal(world, tx - 6, ty - 6);
            npcDrawed = true;
        }
        paintLast = { tx, ty };
        paintShiftPrev = app->shiftPressed;
        return;
    }

    // Bonus
    if (m == Material::NpcBonusCell) {
        if (!npcDrawed) {
            npcs.AddBonus(world, tx - 6, ty - 6);
            npcDrawed = true;
        }
        paintLast = { tx, ty };
        paintShiftPrev = app->shiftPressed;
        return;
    }

    auto stampCircle = [&](int pcx, int pcy, int& minX, int& minY, int& maxX, int& maxY) {
        const int rr = std::fmax(1, r);
        const int r2 = rr * rr;

        if (m == Material::Empty) {
            npcs.EraseEntitiesInCircle(world, pcx, pcy, rr, true, true, true, true);
        }
        const int xmin = std::fmax(0, pcx - rr);
        const int xmax = std::fmin(world.GridW() - 1, pcx + rr);
        const int ymin = std::fmax(0, pcy - rr);
        const int ymax = std::fmin(world.GridH() - 1, pcy + rr);

        minX = std::fmin(minX, xmin);
        minY = std::fmin(minY, ymin);
        maxX = std::fmax(maxX, xmax);
        maxY = std::fmax(maxY, ymax);

        for (int y = ymin; y <= ymax; ++y) {
            for (int x = xmin; x <= xmax; ++x) {
                const int dx = x - pcx;
                const int dy = y - pcy;
                if (dx * dx + dy * dy <= r2) {
                    const Cell oldCell = world.GetFrontCell(x, y);
                    if (engine.levelCellsProtection && oldCell.fromLevel == 1) {
                        continue;
                    }

                    if (oldCell.m != m) {
                        world.SetFrontCell(x, y, Cell{ (uint8)m, 0 });
                        if (oldCell.m != m && m != Material::Empty) {
                            engine.AddMaterialUse(1);
                        }
                    }
                }
            }
        }
    };

    //El pintado mas clean
    auto paintSegment = [&](int x0, int y0, int x1, int y1,
        int& minX, int& minY, int& maxX, int& maxY)
        {
            const int rr = std::fmax(1, r);
            const int stride = std::fmax(1, rr / 2);

            int dx = std::abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
            int dy = -std::abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
            int err = dx + dy;

            int step = 0;
            for (;;) {
                if (step % stride == 0) {
                    stampCircle(x0, y0, minX, minY, maxX, maxY);
                }

                if (x0 == x1 && y0 == y1) break;

                const int e2 = 2 * err;
                if (e2 >= dy) { err += dy; x0 += sx; }
                if (e2 <= dx) { err += dx; y0 += sy; }
                ++step;
            }
            stampCircle(x1, y1, minX, minY, maxX, maxY);
        };

    int minX = world.GridW(), minY = world.GridH(), maxX = 0, maxY = 0;
    paintSegment(paintLast.x, paintLast.y, tx, ty, minX, minY, maxX, maxY);

    if (minX <= maxX && minY <= maxY) {
        world.MarkChunksInRect(minX, minY, (maxX - minX) + 1, (maxY - minY) + 1);
    }

    paintLast = { tx, ty };
    paintShiftPrev = app->shiftPressed;

    //Sonido
    if (engine.app && engine.app->audio) {
        if (!paintInstance) {
            paintInstance = engine.app->audio->Play("paint", 0, 0, 1.0, true);
        }
        else {
            if (!engine.app->audio->IsPlaying("paint", paintInstance)) {
                engine.app->audio->Resume("paint", paintInstance);
            }
            engine.app->audio->SetVoicePosition("paint", paintInstance, tx, 0);
        }
    }
}

void PaintTool::StopPaint(Engine& engine)
{
    if (engine.app && engine.app->audio) {
        if (paintInstance && engine.app->audio->IsPlaying("paint", paintInstance)) {
            engine.app->audio->Pause("paint", paintInstance);
        }
    }

    paintActive = false;
    paintShiftPrev = false;
    paintAxis = PaintAxis::None;
    npcDrawed = false;
}
