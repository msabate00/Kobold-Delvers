#pragma once
#include "app/module.h"
#include <vector>
#include <cstdint>
#include "material.h"
#include "level.h"
#include "worldsim.h"
#include "paint_tool.h"
#include "npc_system.h"
#include "player_system.h"
#include "level_io.h"
#include <app/timer.h>


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
    void ResizeGrid(int w, int h, bool keepContent = false);

    void ClearWorld(uint8 fill = (uint8)Material::Empty);

    bool InRange(int x, int y) const { return world.InRange(x, y); }

    Cell GetCell(int x, int y) { return world.GetFrontCell(x, y); }

    //Material rules
    bool tryMove(int x0, int y0, int x1, int y1, const Cell& c);
    bool trySwap(int x0, int y0, int x1, int y1, const Cell& c);
    void SetCell(int x, int y, uint8 m);

    //RNG Determinsita
    bool randbit(int x, int y, int parity);
    uint32_t randu32(int x, int y, int parity, uint32_t salt = 0);
    int randrange(int x, int y, int parity, int mod, uint32_t salt = 0);

    //Pintado
    void Paint(int screenX, int screenY, Material m, int r);
    void StopPaint();
    void AddMaterialUse(int amount);
    void ResetLevelSession();

    //Chunks
    const std::vector<uint>& GetChunks() const { return world.ChunksDirtyNow(); }
    void GetChunkRect(int chunkIndex, int& x, int& y, int& w, int& h) { world.GetChunkRect(chunkIndex, x, y, w, h); }
    bool PopChunkDirtyGPURect(int& x, int& y, int& rw, int& rh) { return world.PopChunkDirtyGPURect(x, y, rw, rh); }
    void MarkChunkRect(int& x, int& y, int& rw, int& rh){ world.MarkChunksInRect(x, y, rw, rh); }

    //Entidades

    const std::vector<NPC>& GetNPCs() const { return npcs.GetNPCs(); }
    const std::vector<NPCSpawner>& GetSpawners() const { return npcs.GetSpawners(); }
    const std::vector<NPCGoal>& GetGoals() const { return npcs.GetGoals(); }
    const std::vector<NPCBonus>& GetBonuses() const { return npcs.GetBonuses(); }
    bool HasPlayer() const { return players.HasPlayer(); }
    const Player* GetPlayer() const { return players.GetPlayer(); }
    bool PlayerDeathFinished() const { return players.PlayerDeathFinished(); }
    bool BonusTriggered() const { return npcs.AnyBonusClaimed(); }
    int CapturedGoalCount() const { return npcs.TotalCapturedCount(); }

    //Levels
    void ExportLevel(Level& out) const { levelio.ExportLevel(world, npcs, players, out); }
    bool ImportLevel(const Level& in) { return levelio.ImportLevel(*this, world, npcs, players, in); }
    bool SaveLevel(const char* path) const { return levelio.SaveLevel(world, npcs, players, path); }
    bool LoadLevel(const char* path) { return levelio.LoadLevel(*this, world, npcs, players, path); }

    void SetLevelMaterialLimits(int maxUse, int starUse);
    int LevelMaterialBudgetMax() const { return levelMaterialBudgetMax; }
    int LevelMaterialBudgetStar() const { return levelMaterialBudgetStar; }
    int MaterialUsed() const { return materialUsed; }
    bool MaterialBudgetEnabled() const { return levelMaterialBudgetMax > 0; }
    void SetEditorPlayerTriggerMaterial(Material m);
    Material GetEditorPlayerTriggerMaterial() const { return editorPlayerTriggerMaterial; }

    //Dimensiones
    bool WorldRectToScreen(float x, float y, float w, float h, int vw, int vh, float& sx, float& sy, float& sw, float& sh);
    bool ScreenToWorldCell(int inX, int inY, int& outX, int& outY) const;
    bool GetPlayerPlaceTargetCellFromMouse(int mouseScreenX, int mouseScreenY, int& outX, int& outY) const;

public:
    void CycleSimSpeed();
public:
    bool paused = false;
    bool stepOnce = false;
    int simSpeed = 1;
    int parity = 0;

    bool levelCellsProtection = false;
    Material brushMat;

    Timer sceneTimer;

private:
    WorldSim world;
    PaintTool paint;
    void RebuildOcc();

    NPCSystem npcs;
    PlayerSystem players;
    LevelIO levelio;

    int levelMaterialBudgetMax = 0;
    int levelMaterialBudgetStar = 0;
    int materialUsed = 0;
    Material editorPlayerTriggerMaterial = Material::Sand;
};
