#include "worldsim.h"
#include "app/app.h"
#include "engine.h" 
#include "app/log.h"
#include <algorithm>
#include <cmath>

bool WorldSim::Awake(App* app)
{
    if (!app) {
        LOG("ERROR: WorldSim::Awake called with null App*");
        return false;
    }
    Resize(app, app->gridSize.x, app->gridSize.y);
    return true;
}

void WorldSim::Resize(App* app, int w, int h, bool keepContent)
{
    (void)app;

    const int oldW = gridW;
    const int oldH = gridH;

    gridW = std::fmax(CHUNK_SIZE, w);
    gridH = std::fmax(CHUNK_SIZE, h);

    std::vector<Cell> oldFront;
    std::vector<uint8> oldMFront;

    if (keepContent && oldW > 0 && oldH > 0 && !front.empty() && !mFront.empty()) {
        oldFront = std::move(front);
        oldMFront = std::move(mFront);
    }


    chunksW = (gridW + CHUNK_SIZE - 1) / CHUNK_SIZE;
    chunksH = (gridH + CHUNK_SIZE - 1) / CHUNK_SIZE;

    const size_t n = static_cast<size_t>(gridW) * static_cast<size_t>(gridH);
    front.assign(n, Cell{ (uint8)Material::Empty});
    back.assign(n, Cell{ (uint8)Material::Null});
    mFront.assign(n, (uint8)Material::Empty);
    mBack.assign(n, (uint8)Material::Empty);


    if (!oldFront.empty() && !oldMFront.empty()) {
        const int cw = std::fmin(oldW, gridW);
        const int ch = std::fmin(oldH, gridH);
        for (int y = 0; y < ch; ++y) {
            for (int x = 0; x < cw; ++x) {
                const int oi = y * oldW + x;
                const int ni = y * gridW + x;
                front[ni] = oldFront[oi];
                mFront[ni] = oldMFront[oi];
            }
        }
    }

    const size_t cn = static_cast<size_t>(chunksW) * static_cast<size_t>(chunksH);
    chunkDirtyNow.assign(cn, 1);
    chunkDirtyNext.assign(cn, 0);
    chunkDirtyGPU.assign(cn, 1);
    chunkTTL.assign(cn, 0);

    elapsedTimeSinceStep = 0.0f;

    LOG("World resized to %dx%d (chunks %dx%d)", gridW, gridH, chunksW, chunksH);
}

void WorldSim::ResetDirtyAll()
{
    const size_t cn = static_cast<size_t>(chunksW) * static_cast<size_t>(chunksH);
    if (cn == 0) return;
    chunkDirtyNow.assign(cn, 1);
    chunkDirtyNext.assign(cn, 0);
    chunkDirtyGPU.assign(cn, 1);
    chunkTTL.assign(cn, 0);
}

void WorldSim::Clear(uint8 fill)
{
    const size_t n = static_cast<size_t>(gridW) * static_cast<size_t>(gridH);
    if (n == 0) return;

    front.assign(n, Cell{ fill});
    back.assign(n, Cell{ fill});
    mFront.assign(n, fill);
    mBack.assign(n, fill);

    ResetDirtyAll();

    elapsedTimeSinceStep = 0.0f;
}

void WorldSim::CopyFrontToBack()
{
    back = front;
    mBack = mFront;
}

void WorldSim::SwapBuffers()
{
    front.swap(back);
    mFront.swap(mBack);
}

Cell WorldSim::GetFrontCell(int x, int y) const
{
    return (InRange(x, y)) ? front[LinearIndex(x, y)] : Cell{ (uint8)Material::Null };
}

void WorldSim::SetFrontCell(int x, int y, uint8 m)
{
    if (!InRange(x, y)) return;
    const int i = LinearIndex(x, y);
    if (front[i].m == m) return;
    front[i].m = m;
    mFront[i] = m;
}

bool WorldSim::TryMove(int x0, int y0, int dx, int dy, const Cell& c, const std::vector<int>& occ)
{
    const int nx = x0 + dx;
    const int ny = y0 + dy;
    if (!InRange(nx, ny)) return false;

    const int si = LinearIndex(x0, y0);
    const int ni = LinearIndex(nx, ny);

    if (back[ni].m != (uint8)Material::Empty) return false;
    if (!occ.empty() && occ[ni] != 0) return false;

    back[ni] = c;
    mBack[ni] = c.m;
    if (back[si].m == front[si].m) {
        back[si].m = (uint8)Material::Empty;
        mBack[si] = (uint8)Material::Empty;
    }

    MarkChunkSim(x0, y0); MarkChunkSim(nx, ny);
    MarkChunkGPU(x0, y0); MarkChunkGPU(nx, ny);
    MarkChunksNeighborIfBorder(x0, y0);
    MarkChunksNeighborIfBorder(nx, ny);

    // TTL
    const int c0 = ChunkIndexByCell(x0, y0);
    const int c1 = ChunkIndexByCell(nx, ny);
    if (c0 >= 0) chunkTTL[c0] = std::max<uint8_t>(chunkTTL[c0], isVolatile(back[ni].m) ? TTL_VOL : TTL_MIN);
    if (c1 >= 0) chunkTTL[c1] = std::max<uint8_t>(chunkTTL[c1], isVolatile(back[si].m) ? TTL_VOL : TTL_MIN);

    return true;
}

bool WorldSim::TrySwap(int x0, int y0, int dx, int dy, const Cell& c, const std::vector<int>& occ)
{
    const int nx = x0 + dx;
    const int ny = y0 + dy;
    if (!InRange(nx, ny)) return false;

    const int si = LinearIndex(x0, y0);
    const int ni = LinearIndex(nx, ny);
    if (si == ni) return false;

    const Cell& dst = front[ni];
    if (dst.m == (uint8)Material::Empty) return false;
    if (!occ.empty() && occ[ni] != 0) return false;
    if (matProps(c.m).density <= matProps(dst.m).density) return false;

    back[ni] = c;
    back[si] = dst;
    mBack[ni] = c.m;
    mBack[si] = dst.m;

    MarkChunkSim(x0, y0); MarkChunkSim(nx, ny);
    MarkChunkGPU(x0, y0); MarkChunkGPU(nx, ny);
    MarkChunksNeighborIfBorder(x0, y0);
    MarkChunksNeighborIfBorder(nx, ny);

    const int c0 = ChunkIndexByCell(x0, y0);
    const int c1 = ChunkIndexByCell(nx, ny);
    if (c0 >= 0) chunkTTL[c0] = std::max<uint8_t>(chunkTTL[c0], isVolatile(dst.m) ? TTL_VOL : TTL_MIN);
    if (c1 >= 0) chunkTTL[c1] = std::max<uint8_t>(chunkTTL[c1], isVolatile(c.m) ? TTL_VOL : TTL_MIN);

    return true;
}

void WorldSim::SetCell(int x, int y, uint8 m)
{
    if (!InRange(x, y)) return;
    const int i = LinearIndex(x, y);
    if (back[i].m == m) return;

    back[i].m = m;
    mBack[i] = m;

    MarkChunkSim(x, y);
    MarkChunkGPU(x, y);
    MarkChunksNeighborIfBorder(x, y);

    const int c0 = ChunkIndexByCell(x, y);
    if (c0 >= 0) chunkTTL[c0] = std::max<uint8_t>(chunkTTL[c0], isVolatile(m) ? TTL_VOL : TTL_MIN);
}

void WorldSim::Step(Engine& engine, int parity)
{
    for (int cy = chunksH - 1; cy >= 0; cy--) {
        const bool cl2r = ((cy ^ parity) & 1);
        const int cx0 = cl2r ? 0 : (chunksW - 1);
        const int cx1 = cl2r ? chunksW : -1;
        const int cinc = cl2r ? 1 : -1;

        for (int cx = cx0; cx != cx1; cx += cinc) {
            const int ci = ChunkLinearIndex(cx, cy);
            if (!chunkDirtyNow[ci]) continue;

            bool volatileSeen = false;

            int cX, cY, cW, cH;
            GetChunkRect(ci, cX, cY, cW, cH);
            const int x0 = cX;
            const int y0 = cY;
            const int x1 = cX + cW;
            const int y1 = cY + cH;

            for (int y = y1 - 1; y >= y0; y--) {
                const bool l2r = ((y ^ parity) & 1);
                const int xs = l2r ? x0 : (x1 - 1);
                const int xe = l2r ? x1 : (x0 - 1);
                const int inc = l2r ? 1 : -1;

                for (int x = xs; x != xe; x += inc) {
                    const Cell c = front[LinearIndex(x, y)];
                    if (c.m == (uint8)Material::Empty) continue;
                    if (isVolatile(c.m)) volatileSeen = true;

                    const MatProps& mp = matProps(c.m);
                    if (mp.update) mp.update(engine, x, y, c);
                }
            }

            if (volatileSeen) chunkTTL[ci] = std::max<uint8>(chunkTTL[ci], TTL_VOL);
        }
    }

    std::fill(chunkDirtyNow.begin(), chunkDirtyNow.end(), 0);
    chunkDirtyNow.swap(chunkDirtyNext);
    std::fill(chunkDirtyNext.begin(), chunkDirtyNext.end(), 0);

    for (int ci = 0; ci < (int)chunkTTL.size(); ++ci) {
        if (chunkTTL[ci]) {
            chunkDirtyNow[ci] = 1;
            --chunkTTL[ci];
        }
    }
}

int WorldSim::ChunkIndexByCell(int x, int y) const
{
    if (!InRange(x, y)) return -1;
    const int chunkX = x / CHUNK_SIZE;
    const int chunkY = y / CHUNK_SIZE;
    return chunkY * chunksW + chunkX;
}

void WorldSim::MarkChunkSim(int x, int y)
{
    const int ci = ChunkIndexByCell(x, y);
    if (ci >= 0) chunkDirtyNext[ci] = 1;
}

void WorldSim::MarkChunkSim(int ci)
{
    if (ci >= 0) chunkDirtyNext[ci] = 1;
}

void WorldSim::MarkChunkGPU(int x, int y)
{
    const int ci = ChunkIndexByCell(x, y);
    if (ci >= 0) chunkDirtyGPU[ci] = 1;
}

void WorldSim::MarkChunkGPU(int ci)
{
    if (ci >= 0) chunkDirtyGPU[ci] = 1;
}

void WorldSim::MarkChunksInRect(int x, int y, int w, int h)
{
    int minX = (int)std::fmax(0.0f, (float)x);
    int minY = (int)std::fmax(0.0f, (float)y);
    int maxX = (int)std::fmin((float)gridW, (float)(x + w));
    int maxY = (int)std::fmin((float)gridH, (float)(y + h));

    int minCX = minX / CHUNK_SIZE;
    int minCY = minY / CHUNK_SIZE;
    int maxCX = (maxX + CHUNK_SIZE - 1) / CHUNK_SIZE;
    int maxCY = (maxY + CHUNK_SIZE - 1) / CHUNK_SIZE;

    minCX = std::fmax(0, minCX);
    minCY = std::fmax(0, minCY);
    maxCX = std::fmin(chunksW, maxCX);
    maxCY = std::fmin(chunksH, maxCY);

    for (int cy = minCY; cy < maxCY; cy++) {
        for (int cx = minCX; cx < maxCX; cx++) {
            const int ci = cy * chunksW + cx;
            MarkChunkSim(ci);
            MarkChunkGPU(ci);
            chunkDirtyNow[ci] = 1;
        }
    }
}

void WorldSim::MarkChunksNeighborIfBorder(int x, int y)
{
    const int cx = x % CHUNK_SIZE;
    const int cy = y % CHUNK_SIZE;

    if (cx == 0) MarkChunkSim(x - 1, y);
    else if (cx == CHUNK_SIZE - 1) MarkChunkSim(x + 1, y);

    if (cy == 0) MarkChunkSim(x, y - 1);
    else if (cy == CHUNK_SIZE - 1) MarkChunkSim(x, y + 1);
}

void WorldSim::GetChunkRect(int ci, int& x, int& y, int& w, int& h) const
{
    if (ci < 0) { x = y = w = h = 0; return; }

    const int cx = ci % chunksW;
    const int cy = ci / chunksW;
    x = cx * CHUNK_SIZE;
    y = cy * CHUNK_SIZE;
    w = (int)std::fmin((float)gridW, (float)(x + CHUNK_SIZE)) - x;
    h = (int)std::fmin((float)gridH, (float)(y + CHUNK_SIZE)) - y;
}

bool WorldSim::PopChunkDirtyGPURect(int& x, int& y, int& rw, int& rh)
{
    for (int cy = 0; cy < chunksH; cy++) {
        int cx = 0;
        while (cx < chunksW) {
            while (cx < chunksW && !chunkDirtyGPU[cy * chunksW + cx]) cx++;
            if (cx >= chunksW) break;
            const int sx = cx;
            while (cx < chunksW && chunkDirtyGPU[cy * chunksW + cx]) {
                chunkDirtyGPU[cy * chunksW + cx] = 0;
                cx++;
            }
            x = sx * CHUNK_SIZE;
            y = cy * CHUNK_SIZE;
            rw = (int)std::fmin((float)(gridW - x), (float)((cx - sx) * CHUNK_SIZE));
            rh = (int)std::fmin((float)(gridH - y), (float)CHUNK_SIZE);
            return true;
        }
    }
    x = y = rw = rh = 0;
    return false;
}
