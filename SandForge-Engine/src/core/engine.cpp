#include "engine.h"
#include "app/app.h"
#include "input.h"
#include "ui/ui.h"
#include "render/renderer.h"
#include "level.h"
#include <algorithm>
#include <cmath>

Engine::Engine(App* app, bool start_enabled) : Module(app, start_enabled) {};
Engine::~Engine() = default;


void Engine::ClearWorld(uint8 fill)
{
    const size_t n = static_cast<size_t>(gridW) * static_cast<size_t>(gridH);
    if (n == 0) return;

    front.assign(n, Cell{ fill, 0 });
    back.assign(n, Cell{ fill, 0 });
    mFront.assign(n, fill);
    mBack.assign(n, fill);
    npcs.clear();
    occ.assign(n, 0);

    // Reset dirty/TTL
    const size_t cn = static_cast<size_t>(chunksW) * static_cast<size_t>(chunksH);
    chunkDirtyNow.assign(cn, 1);
    chunkDirtyNext.assign(cn, 0);
    chunkDirtyGPU.assign(cn, 1);
    chunkTTL.assign(cn, 0);

    elapsedTimeSinceStep = 0.0f;
    parity = 0;
    stepOnce = false;
    npcDrawed = false;

    RebuildOcc();
}


void Engine::ExportLevel(Level& out) const
{
    out.w = gridW;
    out.h = gridH;
    out.grid = mFront;

    out.npcs.clear();
    out.npcs.reserve(npcs.size());
    for (const auto& n : npcs) {
        LevelNPC ln{};
        ln.x = n.x;
        ln.y = n.y;
        ln.w = n.w;
        ln.h = n.h;
        ln.dir = n.dir;
        ln.alive = n.alive;
        out.npcs.push_back(ln);
    }
}


bool Engine::ImportLevel(const Level& in)
{
    if (!in.IsValid()) return false;

    //Reset del grid
    if ((in.w != gridW || in.h != gridH)) {
        gridW = in.w;
        gridH = in.h;
        app->gridSize = { gridW, gridH };

        chunksW = (gridW + CHUNK_SIZE - 1) / CHUNK_SIZE;
        chunksH = (gridH + CHUNK_SIZE - 1) / CHUNK_SIZE;

        const size_t n = static_cast<size_t>(gridW) * static_cast<size_t>(gridH);
        front.assign(n, Cell{ (uint8)Material::Empty, 0 });
        back.assign(n, Cell{ (uint8)Material::Empty, 0 });
        mFront.assign(n, (uint8)Material::Empty);
        mBack.assign(n, (uint8)Material::Empty);
        occ.assign(n, 0);

        const size_t cn = static_cast<size_t>(chunksW) * static_cast<size_t>(chunksH);
        chunkDirtyNow.assign(cn, 1);
        chunkDirtyNext.assign(cn, 0);
        chunkDirtyGPU.assign(cn, 1);
        chunkTTL.assign(cn, 0);
    }

    //Materiales
    const size_t n = static_cast<size_t>(gridW) * static_cast<size_t>(gridH);
    for (size_t i = 0; i < n; ++i) {
        uint8 m = in.grid[i];
        front[i].m = m;
        back[i].m = m;
        mFront[i] = m;
        mBack[i] = m;
    }

    //Npcs
    npcs.clear();
    for (const auto& ln : in.npcs) {
        if (!ln.alive) continue;
        AddNPC(ln.x, ln.y, ln.dir);
        npcs.back().alive = ln.alive;
    }

    //Chunks
    std::fill(chunkDirtyNow.begin(), chunkDirtyNow.end(), 1);
    std::fill(chunkDirtyNext.begin(), chunkDirtyNext.end(), 0);
    std::fill(chunkDirtyGPU.begin(), chunkDirtyGPU.end(), 1);
    std::fill(chunkTTL.begin(), chunkTTL.end(), 0);

    elapsedTimeSinceStep = 0.0f;
    npcDrawed = false;
    RebuildOcc();

    //Camera/rescale
    app->SetCameraRect(app->camera.pos.x, app->camera.pos.y, app->camera.size.x, app->camera.size.y);
 
    return true;
}


bool Engine::SaveLevel(const char* path) const
{
    if (!path || !path[0]) return false;
    Level lvl;
    ExportLevel(lvl);
    return SaveLevelFile(path, lvl);
}


bool Engine::LoadLevel(const char* path)
{
    if (!path || !path[0]) return false;
    Level lvl;
    if (!LoadLevelFile(path, lvl)) return false;
    return ImportLevel(lvl);
}




bool Engine::Awake() { 

	gridW = app->gridSize.x;
	gridH = app->gridSize.y;

    chunksW = (gridW + CHUNK_SIZE - 1) / CHUNK_SIZE;
    chunksH = (gridH + CHUNK_SIZE - 1) / CHUNK_SIZE;

	front.assign(gridW * gridH, Cell{ (uint8)Material::Empty,0 });
	back.assign(gridW * gridH, Cell{ (uint8)Material::Null,0 });
    mFront.assign(gridW * gridH, (uint8)Material::Empty);
    mBack.assign(gridW * gridH, (uint8)Material::Empty);

    occ.assign(gridW * gridH, 0);

    chunkDirtyNow.assign(chunksW * chunksH, 1);
    chunkDirtyNext.assign(chunksW * chunksH, 0);
    chunkDirtyGPU.assign(chunksW * chunksH, 1);
    chunkTTL.assign(chunksW * chunksH, 0);

	return true;

}
bool Engine::Start() {


    //npcTex.Load(SPRITE_DIR  "/SpritesTest.png");
    npcTex.Load(SPRITE_DIR  "/KoboldMiner.png");

    // NPC animations
    npcAnims.Clear();
    {
        auto& idle = npcAnims.Add("idle");
        idle.defaultTex = &npcTex;
        idle.fps = 6.0f;
        idle.loop = AnimLoopMode::Loop;
        idle.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,0,12,12 }, 0.1f));


        auto& walk = npcAnims.Add("walk");
        walk.defaultTex = &npcTex;
        walk.fps = 6.0f;
        walk.loop = AnimLoopMode::Loop;
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,12,12,12}, 0.1f));
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,12,12,12}, 0.1f));
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,12,12,12}, 0.1f));
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 36,12,12,12}, 0.1f));
        walk.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 48,12,12,12}, 0.1f));

        auto& fall = npcAnims.Add("fall");
        fall.defaultTex = &npcTex;
        fall.fps = 6.0f;
        fall.loop = AnimLoopMode::PingPong;
        fall.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,24,12,12 }, 0.1f));
        fall.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,24,12,12 }, 0.1f));
        fall.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,24,12,12 }, 0.1f));


        auto& die = npcAnims.Add("die");
        die.defaultTex = &npcTex;
        die.fps = 6.0f;
        die.loop = AnimLoopMode::None;
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 12,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 24,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 36,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 48,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 60,36,12,12 }, 0.1f));
        die.frames.push_back(AnimFramePx(&npcTex, AtlasRectPx{ 0,0,0,0 }, 0.1f));
    }

    return true; 
}
bool Engine::PreUpdate() { return true; }

bool Engine::Update(float dt) { 
	
	elapsedTimeSinceStep += dt;



    while (elapsedTimeSinceStep >= fixedTimeStep && (!paused || stepOnce)) {
        back = front;
        mBack = mFront;

        RebuildOcc();
        Step();
        MoveNPCs();
        RebuildOcc();


        front.swap(back);
        mFront.swap(mBack);

        elapsedTimeSinceStep -= fixedTimeStep;
        parity ^= 1;

        if (paused) { stepOnce = false; break; }
    }
    AnimateNPCs(dt);

    if (paused) elapsedTimeSinceStep = 0;

	return true;
}
bool Engine::PostUpdate() { return true; }
bool Engine::CleanUp() { return true; }



bool Engine::tryMove(int x0, int y0, int x1, int y1, const Cell& c)
{

    int nx = x0 + x1, ny = y0 + y1;
    if (!InRange(nx, ny)) return false;
    int si = LinearIndex(x0, y0), ni = LinearIndex(nx, ny);

    if (back[ni].m != (uint8)Material::Empty) return false;
    if (occ[ni] != 0) return false;
   

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

    //Chunk TTL
    int c0 = ChunkIndexByCell(x0, y0);
    int c1 = ChunkIndexByCell(nx, ny);
    if (c0 >= 0) chunkTTL[c0] = std::max<uint8_t>(chunkTTL[c0], isVolatile(back[ni].m) ? TTL_VOL : TTL_MIN);
    if (c1 >= 0) chunkTTL[c1] = std::max<uint8_t>(chunkTTL[c1], isVolatile(back[si].m) ? TTL_VOL : TTL_MIN);

    return true;
}

bool Engine::trySwap(int x0, int y0, int x1, int y1, const Cell& c)
{
    int nx = x0 + x1, ny = y0 + y1;
    if (!InRange(nx, ny)) return false;

    int si = LinearIndex(x0, y0);
    int ni = LinearIndex(nx, ny);
    if (si == ni) return false;

    const Cell& dst = front[ni];

    if (dst.m == (uint8)Material::Empty) return false; //No se puede intercambiar con empties
    if (occ[ni] != 0) return false;
    if (matProps(c.m).density <= matProps(dst.m).density) return false; //No se puede intercambiar por densidad //Lo podria quitar realmetne


    back[ni] = c;
    back[si] = dst;
    mBack[ni] = c.m;
    mBack[si] = dst.m;

    MarkChunkSim(x0, y0); MarkChunkSim(nx, ny);
    MarkChunkGPU(x0, y0); MarkChunkGPU(nx, ny);


    MarkChunksNeighborIfBorder(x0, y0);
    MarkChunksNeighborIfBorder(nx, ny);

    //Chunk TTL
    int c0 = ChunkIndexByCell(x0, y0);
    int c1 = ChunkIndexByCell(nx, ny);
    if (c0 >= 0) chunkTTL[c0] = std::max<uint8_t>(chunkTTL[c0], isVolatile(dst.m) ? TTL_VOL : TTL_MIN);
    if (c1 >= 0) chunkTTL[c1] = std::max<uint8_t>(chunkTTL[c1], isVolatile(c.m) ? TTL_VOL : TTL_MIN);

    return true;
}

void Engine::SetCell(int x, int y, uint8 m)
{
    if (!InRange(x, y)) return;
    int i = LinearIndex(x, y);
    if (back[i].m == m) return;

    back[i].m = m; 
    mBack[i] = m;

    MarkChunkSim(x, y);
    MarkChunkGPU(x, y);
    MarkChunksNeighborIfBorder(x, y);

    //Chunk TTL
    int c0 = ChunkIndexByCell(x, y);
    if (c0 >= 0) chunkTTL[c0] = std::max<uint8_t>(chunkTTL[c0], isVolatile(m) ? TTL_VOL : TTL_MIN);
}



void Engine::Step() {

    for (int cy = chunksH - 1; cy >= 0; cy--) {
        bool cl2r = ((cy ^ parity) & 1);
        int cx0 = cl2r ? 0 : (chunksW - 1);
        int cx1 = cl2r ? chunksW : -1;
        int cinc = cl2r ? 1 : -1;

        for (int cx = cx0; cx != cx1; cx += cinc) {
            int ci = ChunkLinearIndex(cx, cy);
            if (!chunkDirtyNow[ci]) continue;

            bool volatileSeen = false;

            int cX, cY, cW, cH;
            GetChunkRect(ci, cX, cY, cW, cH);
            int x0 = cX;
            int y0 = cY;
            int x1 = cX + cW;
            int y1 = cY + cH;

            for (int y = y1 - 1; y >= y0; y--) {
                bool l2r = ((y ^ parity) & 1);
                int xs = l2r ? x0 : (x1 - 1);
                int xe = l2r ? x1 : (x0 - 1);
                int inc = l2r ? 1 : -1;

                for (int x = xs; x != xe; x += inc) {
                    const Cell c = front[LinearIndex(x, y)];
                    if (c.m == (uint8)Material::Empty) continue;
                    if (isVolatile(c.m)) volatileSeen = true;

                    const MatProps& mp = matProps(c.m);
                    if (mp.update) mp.update(*this, x, y, c);

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

int Engine::ChunkIndexByCell(int x, int y) const
{

    if (!InRange(x, y)) return -1; //VIGILAR Hay que ver que pasa al devolver -1 en todos los casos
    int chunkX = x / CHUNK_SIZE;
    int chunkY = y / CHUNK_SIZE;

    return chunkY * chunksW + chunkX;
}

void Engine::MarkChunksInRect(int x, int y, int w, int h)
{
    int minX = std::fmax(0, x);
    int minY = std::fmax(0, y);
    int maxX = std::fmin(gridW, x + w);
    int maxY = std::fmin(gridH, y + h);

    //if (minX >= maxX || minY >= maxY) return; //VER QUE PASA SIN ESTO

    int minCX = minX / CHUNK_SIZE;
    int minCY = minY / CHUNK_SIZE;
    int maxCX = (maxX + CHUNK_SIZE - 1) / CHUNK_SIZE;
    int maxCY = (maxY + CHUNK_SIZE - 1) / CHUNK_SIZE;


    if (minCX < 0) minCX = 0;
    if (minCY < 0) minCY = 0;
    if (maxCX > chunksW) maxCX = chunksW;
    if (maxCY > chunksH) maxCY = chunksH;

    for (int cy = minCY; cy < maxCY; cy++) {
        for (int cx = minCX; cx < maxCX; cx++) {

            int ci = cy * chunksW + cx;

            MarkChunkSim(ci);
            MarkChunkGPU(ci);
            chunkDirtyNow[ci] = 1;

        }
    }
}

void Engine::MarkChunksNeighborIfBorder(int x, int y)
{
    int cx = x % CHUNK_SIZE, cy = y % CHUNK_SIZE;


    if (cx == 0) { MarkChunkSim(x - 1, y); } 
    else if (cx == CHUNK_SIZE - 1) { MarkChunkSim(x + 1, y); }

    if (cy == 0) { MarkChunkSim(x, y - 1); }
    else if (cy == CHUNK_SIZE - 1) { MarkChunkSim(x, y + 1); }

}

void Engine::GetChunkRect(int ci, int& x, int& y, int& w, int& h)
{
    if (ci < 0) { x = y = w = h = 0; return; }

    int cx = ci % chunksW;
    int cy = ci / chunksW;
    x = cx * CHUNK_SIZE;  
    y = cy * CHUNK_SIZE;
    w = std::fmin(gridW, x + CHUNK_SIZE) - x;
    h = std::fmin(gridH, y + CHUNK_SIZE) - y;
}

bool Engine::PopChunkDirtyGPURect(int& x, int& y, int& rw, int& rh)
{
    for (int cy = 0; cy < chunksH; cy++) {
        int cx = 0;
        while (cx < chunksW) {
            while (cx < chunksW && !chunkDirtyGPU[cy * chunksW + cx]) cx++;
            if (cx >= chunksW) break;
            int sx = cx;
            while (cx < chunksW && chunkDirtyGPU[cy * chunksW + cx]) {
                chunkDirtyGPU[cy * chunksW + cx] = 0;
                cx++;
            }
            x = sx * CHUNK_SIZE;
            y = cy * CHUNK_SIZE;
            rw = std::fmin(gridW - x, (cx - sx) * CHUNK_SIZE);
            rh = std::fmin(gridH - y, CHUNK_SIZE);
            return true;
        }
    }
    x = y = rw = rh = 0;
    return false;
}

void Engine::Paint(int sx, int sy, Material m, int r, bool shift) {

    int cx = 0, cy = 0;
    if (!ScreenToWorldCell(sx, sy, cx, cy)) return;

    if (!paintActive) {
        paintActive = true;
        paintAnchor = { cx, cy };
        paintLast = { cx, cy };
        paintAxis = PaintAxis::None;
        paintShiftPrev = shift;
    }

    if (shift && !paintShiftPrev) {
        paintAnchor = paintLast;
        paintAxis = PaintAxis::None;
    }

    int tx = cx, ty = cy;
    if (shift) {
        const int dx = tx - paintAnchor.x;
        const int dy = ty - paintAnchor.y;

        const int adx = std::abs(dx);
        const int ady = std::abs(dy);
        constexpr int kCommitCells = 4; //Distancia para decidir el eje

        auto pickAxis = [&]() {
            if (adx == 0 && ady == 0) return;
            if (adx > ady) paintAxis = PaintAxis::X;
            else if (ady > adx) paintAxis = PaintAxis::Y;

        };

        if (paintAxis == PaintAxis::None) {
            pickAxis();
        }

        if (paintAxis == PaintAxis::X) ty = paintAnchor.y;
        else if (paintAxis == PaintAxis::Y) tx = paintAnchor.x;
    }
    else {
        paintAxis = PaintAxis::None;
    }

    //NPC
    if (m == Material::NpcCell) {
        if (!npcDrawed) {
            AddNPC(tx-5, ty-9);
            npcDrawed = true;
        }
        paintLast = { tx, ty };
        paintShiftPrev = shift;
        return;
    }

    
    auto stampCircle = [&](int pcx, int pcy, int& minX, int& minY, int& maxX, int& maxY) {
        const int rr = std::fmax(1, r);
        const int r2 = rr * rr;
        const int xmin = std::fmax(0, pcx - rr);
        const int xmax = std::fmin(gridW - 1, pcx + rr);
        const int ymin = std::fmax(0, pcy - rr);
        const int ymax = std::fmin(gridH - 1, pcy + rr);

        minX = std::fmin(minX, xmin);
        minY = std::fmin(minY, ymin);
        maxX = std::fmax(maxX, xmax);
        maxY = std::fmax(maxY, ymax);

        for (int y = ymin; y <= ymax; ++y) {
            for (int x = xmin; x <= xmax; ++x) {
                const int dx = x - pcx;
                const int dy = y - pcy;
                if (dx * dx + dy * dy <= r2) {
                    const int i = LinearIndex(x, y);
                    front[i].m = (uint8)m;
                    mFront[i] = (uint8)m;
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

    int minX = gridW, minY = gridH, maxX = 0, maxY = 0;
    paintSegment(paintLast.x, paintLast.y, tx, ty, minX, minY, maxX, maxY);

    if (minX <= maxX && minY <= maxY) {
        MarkChunksInRect(minX, minY, (maxX - minX) + 1, (maxY - minY) + 1);
    }

    paintLast = { tx, ty };
    paintShiftPrev = shift;

    if (!paintInstance) {
        paintInstance = app->audio->Play("paint", 0, 0, 1.0, true);
    }
    else {
        if (!app->audio->IsPlaying("paint", paintInstance)) {
            app->audio->Resume("paint", paintInstance);
        }
        app->audio->SetVoicePosition("paint", paintInstance, tx, 0);
    }
}

void Engine::StopPaint()
{
    if (paintInstance && app->audio->IsPlaying("paint", paintInstance)) {
        app->audio->Pause("paint", paintInstance);
    }

    paintActive = false;
    paintShiftPrev = false;
    paintAxis = PaintAxis::None;
}

bool Engine::randbit(int x, int y, int parity) {
    uint32_t h = (uint32_t)(x * 374761393u) ^ (uint32_t)(y * 668265263u) ^ (uint32_t)(parity * 0x9E3779B9u);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    return (h & 1u) != 0u;
}

void Engine::AddNPC(int x, int y, int dir) {

    //Size
    const int w = 12;
    const int h = 12;

    //hitbox
    const int padX = 3;
    const int padTop = 5;
    const int padBottom = 0;

    NPC npc{ x, y, w, h, dir, true };

    npc.hbOffX = std::clamp(padX, 0, (int)std::fmax(0, w - 1));
    npc.hbOffY = std::clamp(padTop, 0, (int)std::fmax(0, h - 1));
    npc.hbW = std::fmax(1, w - 2 * padX);
    npc.hbH = std::fmax(1, h - padTop - padBottom);

    npc.sprite.tex = &npcTex;
    npc.anim.SetLibrary(&npcAnims);
    npc.anim.Play("idle", true);
    npc.anim.ApplyTo(npc.sprite, npc.dir < 0);
    npcs.push_back(npc);

    MarkChunksInRect(x, y, w, h);
}

void Engine::RebuildOcc() {
    std::fill(occ.begin(), occ.end(), 0);
    for (int i = 0; i < (int)npcs.size(); ++i) {
        const auto& n = npcs[i];
        if (!n.alive) continue;
        const int bx = n.x + n.hbOffX;
        const int by = n.y + n.hbOffY;
        for (int yy = 0; yy < n.hbH; ++yy)
            for (int xx = 0; xx < n.hbW; ++xx) {
                int gx = bx + xx, gy = by + yy;
                if (InRange(gx, gy)) occ[LinearIndex(gx, gy)] = i + 1;
            }
    }
}

bool Engine::RectFreeOnBack(int x, int y, int w, int h, int ignoreId) const {
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx) {
            int gx = x + xx, gy = y + yy;
            if (!InRange(gx, gy)) return false;
            int i = LinearIndex(gx, gy);
            if (back[i].m != (uint8)Material::Empty) return false;
            int occId = occ[i];
            if (occId != 0 && occId != ignoreId) return false;
        }
    return true;
}

bool Engine::CheckNPCDie(int x, int y, int w, int h) const
{
    auto isLava = [&](int gx, int gy) -> bool {
        if (!InRange(gx, gy)) return false;
        switch (back[LinearIndex(gx, gy)].m)
        {
        case (uint8)Material::Lava: return true;
        case (uint8)Material::Fire: return true;
        default:
            return false;
            break;
        }
    };

    //Dentro
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx)
            if (isLava(x + xx, y + yy)) return true;

    //Alrededor
    for (int xx = 0; xx < w; ++xx) {
        if (isLava(x + xx, y - 1))   return true; // arriba
        if (isLava(x + xx, y + h))   return true; // abajo
    }
    for (int yy = 0; yy < h; ++yy) {
        if (isLava(x - 1, y + yy))   return true; // izquierda
        if (isLava(x + w, y + yy))   return true; // derecha
    }

    return false;
}

void Engine::MoveNPCs() {

    //Velocidad de movimiento
    npcMoveAcc += fixedTimeStep;
    const float npcMoveInterval = 1.0f / std::fmax(0.001f, npcCellsPerSec);
    if (npcMoveAcc < npcMoveInterval) return;
    npcMoveAcc -= npcMoveInterval;


    constexpr int kMaxStep = 1; 
    for (int i = 0; i < (int)npcs.size(); ++i) {
        auto& n = npcs[i];
        if (!n.alive) continue;
        if (n.anim.CurrentName() == "die") continue;
        int id = i + 1;

        const int hbX = n.x + n.hbOffX;
        const int hbY = n.y + n.hbOffY;

        //Muerte
        if (CheckNPCDie(hbX, hbY, n.hbW, n.hbH)) {
            n.anim.Play("die", false);
            continue;
        }

        //Caer
        if (RectFreeOnBack(hbX, hbY + 1, n.hbW, n.hbH, id)) { n.y += 1; n.anim.Play("fall", false); continue; }

        int nx = n.x + n.dir;

        //Horizontal
        if (RectFreeOnBack(nx + n.hbOffX, hbY, n.hbW, n.hbH, id)) { n.x = nx; n.anim.Play("walk", false); continue; }

        //Trepar
        auto solid = [&](int gx, int gy) {
            return InRange(gx, gy) && back[LinearIndex(gx, gy)].m != (uint8)Material::Empty;
            };
        bool climbed = false;
        for (int step = 1; step <= kMaxStep; ++step) {
            if (!RectFreeOnBack(nx + n.hbOffX, (n.y - step) + n.hbOffY, n.hbW, n.hbH, id)) continue;
            bool support = false;
            const int supY = (n.y - step) + n.hbOffY + n.hbH;
            for (int xx = 0; xx < n.hbW; ++xx)
                support |= solid((nx + n.hbOffX) + xx, supY);
            if (!support) continue;
            n.y -= step; n.x = nx; climbed = true; break;
        }
        if (!climbed) n.dir = -n.dir;
    }
}

void Engine::AnimateNPCs(float dt)
{
    for (auto& n : npcs) {
        
        n.anim.Update(dt);
        n.anim.ApplyTo(n.sprite, n.dir < 0);

        if (n.anim.CurrentName() == "die" && n.anim.IsFinished()) {
            n.alive = false;
        }

        float sx, sy, sw, sh;
        if (!WorldRectToScreen((float)n.x, (float)n.y, (float)n.w, (float)n.h,
            app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh))
            continue;

        n.sprite.x = std::floor(sx);
        n.sprite.y = std::floor(sy);
        n.sprite.w = std::floor(sw);
        n.sprite.h = std::floor(sh);
        n.sprite.layer = RenderLayer::WORLD;
        app->renderer->Queue(n.sprite);
    }

}

 bool Engine::WorldRectToScreen(float x, float y, float w, float h,
    int vw, int vh,
    float& sx, float& sy, float& sw, float& sh){


    const float cx = app->camera.pos.x, cy = app->camera.pos.y;
    const float cw = app->camera.size.x, ch = app->camera.size.y;

    float rx = std::fmax(x, cx);
    float ry = std::fmax(y, cy);
    float rw = std::fmin(x + w, cx + cw) - rx;
    float rh = std::fmin(y + h, cy + ch) - ry;
    if (rw <= 0 || rh <= 0) return false;

    float sxCell = std::floor(vw / cw);
    float syCell = std::floor(vh / ch);
    float s = std::fmax(1.0f, std::fmin(sxCell, syCell));

    float sizeW = cw * s;
    float sizeH = ch * s;
    float offX = (vw - sizeW) * 0.5f;
    float offY = (vh - sizeH) * 0.5f;

    sx = offX + (rx - cx) * s;
    sy = offY + (ry - cy) * s;
    sw = rw * s;
    sh = rh * s;

    sx = std::floor(sx); sy = std::floor(sy);
    sw = std::floor(sw); sh = std::floor(sh);
    return true;
}

 bool Engine::ScreenToWorldCell(int inX, int inY, int& outX, int& outY) const
 {
     
    const float sxCell = std::floor(app->framebufferSize.x / app->camera.size.x);
    const float syCell = std::floor(app->framebufferSize.y / app->camera.size.y);
    const float s = std::fmax(1.0f, std::fmin(sxCell, syCell));

    const float sizeW = app->camera.size.x * s;
    const float sizeH = app->camera.size.y * s;
    const float offX = (app->framebufferSize.x - sizeW) * 0.5f;
    const float offY = (app->framebufferSize.y - sizeH) * 0.5f;

    float fx = (float)inX - offX;
    float fy = (float)inY - offY;

    fx = std::clamp(fx, 0.0f, std::fmax(0.0f, sizeW - 1.0f));
    fy = std::clamp(fy, 0.0f, std::fmax(0.0f, sizeH - 1.0f));

    const float u = (sizeW > 0.0f) ? (fx / sizeW) : 0.0f;
    const float v = (sizeH > 0.0f) ? (fy / sizeH) : 0.0f;

    outX = (int)std::floor(app->camera.pos.x + u * app->camera.size.x);
    outY = (int)std::floor(app->camera.pos.y + v * app->camera.size.y);
    outX = std::clamp(outX, 0, gridW - 1);
    outY = std::clamp(outY, 0, gridH - 1);

    return true;
         
 }
