#pragma once
#include <vector>
#include <cstdint>
#include "material.h"

struct App;
struct Engine;

class WorldSim
{
public:
    bool Awake(App* app);

    void Resize(App* app, int w, int h, bool keepContent = false);

    void Clear(uint8 fill = (uint8)Material::Empty);

    void ResetDirtyAll();

    int GridW() const { return gridW; }
    int GridH() const { return gridH; }

    const uint8* FrontPlane() const { return mFront.data(); }

    void CopyFrontToBack();
    void SwapBuffers();

    bool InRange(int x, int y) const { return x >= 0 && x < gridW && y >= 0 && y < gridH; }
    int LinearIndex(int x, int y) const { return y * gridW + x; }

    Cell GetFrontCell(int x, int y) const;
    const Cell& FrontAtIndex(int i) const { return front[i]; }
    const Cell& BackAtIndex(int i) const { return back[i]; }
    uint8 BackMatAtIndex(int i) const { return back[i].m; }

    void SetFrontCell(int x, int y, uint8 m);
    void SetFrontCell(int x, int y, const Cell& c);

    void Step(Engine& engine, int parity);

    bool TryMove(int x0, int y0, int dx, int dy, const Cell& c, const std::vector<int>& occ);
    bool TrySwap(int x0, int y0, int dx, int dy, const Cell& c, const std::vector<int>& occ);
    void SetCell(int x, int y, uint8 m);

    const std::vector<uint>& ChunksDirtyNow() const { return chunkDirtyNow; }
    void GetChunkRect(int chunkIndex, int& x, int& y, int& w, int& h) const;
    bool PopChunkDirtyGPURect(int& x, int& y, int& rw, int& rh);
    void MarkChunksInRect(int x, int y, int w, int h);

    float& StepAccumulator() { return elapsedTimeSinceStep; }
    float FixedTimeStep() const { return fixedTimeStep; }

private:
    int ChunkLinearIndex(int x, int y) const { return y * chunksW + x; }
    int ChunkIndexByCell(int x, int y) const;

    void MarkChunkSim(int x, int y);
    void MarkChunkSim(int ci);
    void MarkChunkGPU(int x, int y);
    void MarkChunkGPU(int ci);
    void MarkChunksNeighborIfBorder(int x, int y);

private:
    std::vector<Cell> front, back;
    std::vector<uint8> mFront, mBack;

    std::vector<uint> chunkDirtyNow, chunkDirtyNext;
    std::vector<uint> chunkDirtyGPU;
    std::vector<uint8> chunkTTL;

    int gridW = 0, gridH = 0;
    int chunksW = 0, chunksH = 0;

    float elapsedTimeSinceStep = 0.0f;

    static constexpr float fixedTimeStep = 1.f / 120.f;
    static constexpr int CHUNK_SIZE = 16;
    static constexpr uint8_t TTL_MIN = 2;
    static constexpr uint8_t TTL_VOL = 32;
};
