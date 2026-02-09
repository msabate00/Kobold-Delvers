#pragma once
#include "app/module.h"
#include <vector>
#include <cstdint>
#include "material.h"
#include "level.h"
#include "audio/audio.h"
#include "render/sprite.h"
#include "render/texture.h"



struct NPC {
    int x, y;
    int w = 2, h = 4;
    int dir = 1;
    bool alive = true;
    Sprite sprite;
};


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

    const uint8* GetFrontPlane() { return mFront.data(); };

    int GridW() const { return gridW; }
    int GridH() const { return gridH; }

    void ExportLevel(Level& out) const;
    bool ImportLevel(const Level& in);
    bool SaveLevel(const char* path) const;
    bool LoadLevel(const char* path);
    void ClearWorld(uint8 fill = (uint8)Material::Empty);


    bool tryMove(int x0, int y0, int x1, int y1, const Cell& c);
    bool trySwap(int x0, int y0, int x1, int y1, const Cell& c);

    void SetCell(int x, int y, uint8 m);

    Cell GetCell(int x, int y) {
        return (InRange(x, y)) ? front[LinearIndex(x, y)] : Cell{ (uint8)Material::Null };
    }

    void Paint(int cx, int cy, Material m, int r);
    void StopPaint();

    bool InRange(int x, int y) const { return x >= 0 && x < gridW && y >= 0 && y < gridH; }
    bool randbit(int x, int y, int parity);

    std::vector<uint> GetChunks() { return chunkDirtyNow; };
    void GetChunkRect(int chunkIndex, int& x, int& y, int& w, int& h);
    bool PopChunkDirtyGPURect(int& x, int& y, int& rw, int& rh);

    void AddNPC(int x, int y, int w = 2, int h = 4, int dir = 1);
    const std::vector<NPC>& GetNPCs() const { return npcs; }

private:

    void Step();

    int LinearIndex(int x, int y) const { return y * gridW + x; };
    int ChunkLinearIndex(int x, int y) const { return y * chunksW + x; };

    int ChunkIndexByCell(int x, int y) const;
    void MarkChunkSim(int x, int y) { int ci = ChunkIndexByCell(x, y); if (ci >= 0) chunkDirtyNext[ci] = 1; }
    void MarkChunkSim(int ci) { if (ci >= 0) chunkDirtyNext[ci] = 1; }
    void MarkChunkGPU(int x, int y) { int ci = ChunkIndexByCell(x, y); if (ci >= 0) chunkDirtyGPU[ci] = 1; }
    void MarkChunkGPU(int ci) { if (ci >= 0) chunkDirtyGPU[ci] = 1; }
    void MarkChunksInRect(int x, int y, int w, int h);
    void MarkChunksNeighborIfBorder(int x, int y);

    void RebuildOcc();
    bool RectFreeOnBack(int x, int y, int w, int h, int ignoreId) const;
    void MoveNPCs();
    bool WorldRectToScreen(float x, float y, float w, float h, int vw, int vh, float& sx, float& sy, float& sw, float& sh);

public:
    bool paused = false;
    bool stepOnce = false;
    int parity = 0;

    bool npcDrawed = false;
     
private:
    std::vector<Cell> front, back;
    std::vector<uint8> mFront, mBack;

    std::vector<uint> chunkDirtyNow, chunkDirtyNext;
    std::vector<uint> chunkDirtyGPU;
    std::vector<uint8> chunkTTL;


    int gridW, gridH;
    int chunksW, chunksH;

    float elapsedTimeSinceStep = 0;
    static constexpr float fixedTimeStep = 1.f / 120.f;
    static constexpr int CHUNK_SIZE = 16;
    static constexpr uint8_t TTL_MIN = 2;         
    static constexpr uint8_t TTL_VOL = 32;

    AudioInstance paintInstance{};

    
    std::vector<NPC> npcs;
    std::vector<int> occ;

    Texture2D npcTex;

};