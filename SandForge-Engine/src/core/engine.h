#pragma once
#include "app/module.h"
#include <vector>
#include <cstdint>
#include "material.h"
#include "level.h"
#include "worldsim.h"
#include "paint_tool.h"
#include "npc_system.h"
#include "level_io.h"


class Engine : public Module {
public:
    Engine(App* app, bool start_enabled = true);
    virtual ~Engine();

    bool Awake();
    bool Start();
    bool PreUpdate();
    bool Update(float dt);
    bool PostUpdate();
    bool CleanUp();

    //World
    const uint8* GetFrontPlane() { return world.FrontPlane(); }
    int GridW() const { return world.GridW(); }
    int GridH() const { return world.GridH(); }

    void ClearWorld(uint8 fill = (uint8)Material::Empty);

    bool InRange(int x, int y) const { return world.InRange(x, y); }

    Cell GetCell(int x, int y) { return world.GetFrontCell(x, y); }

    //Material rules
    bool tryMove(int x0, int y0, int x1, int y1, const Cell& c);
    bool trySwap(int x0, int y0, int x1, int y1, const Cell& c);
    void SetCell(int x, int y, uint8 m);

    bool randbit(int x, int y, int parity);

    //Pintado
    void Paint(int screenX, int screenY, Material m, int r, bool shift);
    void StopPaint();

    //Chunks
    const std::vector<uint>& GetChunks() const { return world.ChunksDirtyNow(); }
    void GetChunkRect(int chunkIndex, int& x, int& y, int& w, int& h) { world.GetChunkRect(chunkIndex, x, y, w, h); }
    bool PopChunkDirtyGPURect(int& x, int& y, int& rw, int& rh) { return world.PopChunkDirtyGPURect(x, y, rw, rh); }

    //NPCs
    NPC& AddNPC(int x, int y, int dir = 1) { return npcs.AddNPC(world, x, y, dir); }
    const std::vector<NPC>& GetNPCs() const { return npcs.GetNPCs(); }

    //Levels
    void ExportLevel(Level& out) const { levelio.ExportLevel(world, npcs, out); }
    bool ImportLevel(const Level& in) { return levelio.ImportLevel(*this, world, npcs, in); }
    bool SaveLevel(const char* path) const { return levelio.SaveLevel(world, npcs, path); }
    bool LoadLevel(const char* path) { return levelio.LoadLevel(*this, world, npcs, path); }

    //Dimensiones
    bool WorldRectToScreen(float x, float y, float w, float h, int vw, int vh, float& sx, float& sy, float& sw, float& sh);
    bool ScreenToWorldCell(int inX, int inY, int& outX, int& outY) const;

public:
    bool paused = false;
    bool stepOnce = false;
    int parity = 0;

private:
    WorldSim world;
    PaintTool paint;
    NPCSystem npcs;
    LevelIO levelio;
};
