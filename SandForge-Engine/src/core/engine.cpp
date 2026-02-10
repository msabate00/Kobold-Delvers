#include "engine.h"
#include "app/app.h"
#include "input.h"
#include "ui/ui.h"
#include "render/renderer.h"
#include "level.h"

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
        AddNPC(ln.x, ln.y, ln.w, ln.h, ln.dir);
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


    npcTex.Load(SPRITE_DIR  "/SpritesTest.png");

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

    if (paused) elapsedTimeSinceStep = 0;

    for (auto& n : npcs) {
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

void Engine::Paint(int cx, int cy, Material m, int r) {


    //if (app->ui->ConsumedMouse()) return;

    float vw = (float)std::fmax(1, app->framebufferSize.x);
    float vh = (float)std::fmax(1, app->framebufferSize.y);
    float cw = std::fmax(1.0f, app->camera.size.x);
    float ch = std::fmax(1.0f, app->camera.size.y);

    float sxCell = std::floor(vw / cw);
    float syCell = std::floor(vh / ch);
    float s = std::fmax(1.0f, std::fmin(sxCell, syCell));
    float sizeW = cw * s;
    float sizeH = ch * s;
    float offX = (vw - sizeW) * 0.5f;
    float offY = (vh - sizeH) * 0.5f;

    // Mouse dentro del área del mundo (letterbox)
    float fx = (float)cx - offX;
    float fy = (float)cy - offY;
    if (fx < 0 || fy < 0 || fx >= sizeW || fy >= sizeH) return;

    float u = fx / sizeW;
    float v = fy / sizeH;

    cx = (int)std::floor(app->camera.pos.x + u * cw);
    cy = (int)std::floor(app->camera.pos.y + v * ch);

    if (m == Material::NpcCell) {
        if (!npcDrawed) {
            AddNPC(cx, cy);
            npcDrawed = true;
        }
        
        return;
    }


    int r2 = r * r;
    int xmin = std::fmax(0, cx - r), xmax = std::fmin(gridW - 1, cx + r);
    int ymin = std::fmax(0, cy - r), ymax = std::fmin(gridH - 1, cy + r);



    for (int y = ymin; y <= ymax; ++y)
        for (int x = xmin; x <= xmax; ++x) {
            int dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r2) {
                int i = LinearIndex(x, y);
                front[i].m = (uint8)m; 
                mFront[i] = (uint8)m;    

            }
        }
    MarkChunksInRect(xmin, ymin, xmax - xmin, ymax - ymin);


    if (!paintInstance) {
        paintInstance = app->audio->Play("paint", 0, 0, 1.0, true);
    }
    else {
        if (!app->audio->IsPlaying("paint", paintInstance)) {
            app->audio->Resume("paint", paintInstance);
        }
        app->audio->SetVoicePosition("paint", paintInstance, cx, 0);
    }
    

}

void Engine::StopPaint()
{
    if (paintInstance && app->audio->IsPlaying("paint", paintInstance)) {
        app->audio->Pause("paint", paintInstance);
    }
}

bool Engine::randbit(int x, int y, int parity) {
    uint32_t h = (uint32_t)(x * 374761393u) ^ (uint32_t)(y * 668265263u) ^ (uint32_t)(parity * 0x9E3779B9u);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    return (h & 1u) != 0u;
}

void Engine::AddNPC(int x, int y, int w, int h, int dir) {
    npcs.push_back(NPC{ x, y, w, h, dir, true });

    Sprite s{};
    s.tex = &npcTex;
    npcs[npcs.size() - 1].sprite = s;

    MarkChunksInRect(x, y, w, h);
}

void Engine::RebuildOcc() {
    std::fill(occ.begin(), occ.end(), 0);
    for (int i = 0; i < (int)npcs.size(); ++i) {
        const auto& n = npcs[i];
        if (!n.alive) continue;
        for (int yy = 0; yy < n.h; ++yy)
            for (int xx = 0; xx < n.w; ++xx) {
                int gx = n.x + xx, gy = n.y + yy;
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

void Engine::MoveNPCs() {
    constexpr int kMaxStep = 1; 
    for (int i = 0; i < (int)npcs.size(); ++i) {
        auto& n = npcs[i];
        if (!n.alive) continue;
        int id = i + 1;

        //Caer
        if (RectFreeOnBack(n.x, n.y + 1, n.w, n.h, id)) { n.y += 1; continue; }

        int nx = n.x + n.dir;

        //Horizontal
        if (RectFreeOnBack(nx, n.y, n.w, n.h, id)) { n.x = nx; continue; }

        //Trepar
        auto solid = [&](int gx, int gy) {
            return InRange(gx, gy) && back[LinearIndex(gx, gy)].m != (uint8)Material::Empty;
            };
        bool climbed = false;
        for (int step = 1; step <= kMaxStep; ++step) {
            if (!RectFreeOnBack(nx, n.y - step, n.w, n.h, id)) continue;
            bool support = false;
            for (int xx = 0; xx < n.w; ++xx)
                support |= solid(nx + xx, (n.y - step) + n.h);
            if (!support) continue;
            n.y -= step; n.x = nx; climbed = true; break;
        }
        if (!climbed) n.dir = -n.dir;
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