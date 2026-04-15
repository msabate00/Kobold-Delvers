#include <array>
#include "material.h"
#include "engine.h"
#include "app/app.h"

static std::array<MatProps, 256> g_mat{};

const MatProps& matProps(uint8 id) { return g_mat[id]; }


static void SandUpdate(Engine& E, int x, int y, const Cell& self) {

    if (E.tryMove(x, y, 0, +1, self)) return; // caer


    if (E.InRange(x, y + 1) && (E.GetCell(x, y + 1).m == (uint8)Material::Water) && E.trySwap(x, y, 0, +1, self)) return;
    if (E.InRange(x, y + 1) && (E.GetCell(x, y + 1).m == (uint8)Material::Lava) && E.trySwap(x, y, 0, +1, self)) return;

    bool leftFirst = E.randbit(x, y, 0);;
    int da = leftFirst ? -1 : +1, db = -da;

    if ((Material)E.GetCell(x + da, y + 1).m == Material::Water && E.trySwap(x, y, da, +1, self)) return;
    if ((Material)E.GetCell(x + da, y + 1).m == Material::Lava && E.trySwap(x, y, da, +1, self)) return;
    if ((Material)E.GetCell(x + db, y + 1).m == Material::Water && E.trySwap(x, y, db, +1, self)) return;
    if ((Material)E.GetCell(x + db, y + 1).m == Material::Lava && E.trySwap(x, y, db, +1, self)) return;

    if (E.tryMove(x, y, da, +1, self)) return;
    E.tryMove(x, y, db, +1, self);
}

static void WaterUpdate(Engine& E, int x, int y, const Cell& self) {
    if (E.tryMove(x, y, 0, +1, self)) return;

    bool leftFirst = E.randbit(x, y, app->frames);
    int da = leftFirst ? -1 : +1, db = -da;

    auto passable = [&](int px, int py) -> bool {
        const uint8 m = E.GetCell(px, py).m;
        return m == (uint8)Material::Empty || m == (uint8)Material::Water;
    };



    // Diagonal
    if (E.GetCell(x + da, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, da, +1, self)) return;
    if (E.GetCell(x + db, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, db, +1, self)) return;

    // Horizontal
    if (E.GetCell(x + da, y).m == (uint8)Material::Empty && E.tryMove(x, y, da, 0, self)) return;
    if (E.GetCell(x + db, y).m == (uint8)Material::Empty && E.tryMove(x, y, db, 0, self)) return;

    //Doble Diagonal
    if (E.GetCell(x + da * 2, y + 2).m == (uint8)Material::Empty &&
        passable(x + da, y + 1) &&
        E.tryMove(x, y, da * 2, +2, self)) return;

    if (E.GetCell(x + db * 2, y + 2).m == (uint8)Material::Empty &&
        passable(x + db, y + 1) &&
        E.tryMove(x, y, db * 2, +2, self)) return;

    //Semi doble diagonal
    if (E.GetCell(x + da * 2, y + 1).m == (uint8)Material::Empty &&
        passable(x + da, y) &&
        E.tryMove(x, y, da * 2, +1, self)) return;

    if (E.GetCell(x + db * 2, y + 1).m == (uint8)Material::Empty &&
        passable(x + db, y) &&
        E.tryMove(x, y, db * 2, +1, self)) return;

    //Doble horizontal
    if (E.GetCell(x + da * 2, y).m == (uint8)Material::Empty &&
        passable(x + da, y) &&
        E.tryMove(x, y, da * 2, 0, self)) return;

    if (E.GetCell(x + db * 2, y).m == (uint8)Material::Empty &&
        passable(x + db, y) &&
        E.tryMove(x, y, db * 2, 0, self)) return;

    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    for (int i = 0; i < 4; ++i) {
        int nx = x + dirs[i][0], ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        if (E.GetCell(nx, ny).m == (uint8)Material::Lava) {
            E.SetCell(x, y, (uint8)Material::Steam);
            E.SetCell(nx, ny, (uint8)Material::Stone);
            return;
        }
    } 
}


static void WoodUpdate(Engine& E, int x, int y, const Cell& self) {
    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    for (int i = 0; i < 4; ++i) {
        int nx = x + dirs[i][0], ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        uint8 mat = E.GetCell(nx, ny).m;
        if (mat == (uint8)Material::Fire || mat == (uint8)Material::Lava) {
            E.SetCell(x, y, (uint8)Material::Fire);
            return; 
        }
    }
}

static void FireUpdate(Engine& E, int x, int y, const Cell& self) {
    if (E.randrange(x, y, app->frames, 100, 1) < 5) {
        E.SetCell(x, y, (uint8)Material::Empty);
        return;
    }

    if (E.InRange(x, y - 1) && E.GetCell(x, y - 1).m == (uint8)Material::Empty) {
        if (E.randrange(x, y, app->frames, 100, 2) < 20) {
            E.SetCell(x, y, (uint8)Material::Smoke);
        }
    }

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if ((dx != 0 || dy != 0) && E.InRange(x + dx, y + dy)) {
                if (E.GetCell(x + dx, y + dy).m == (uint8)Material::Wood) {
                    E.SetCell(x + dx, y + dy, (uint8)Material::Fire);
                }
                if (E.GetCell(x + dx, y + dy).m == (uint8)Material::Water) {
                    E.SetCell(x, y, (uint8)Material::Steam);
                    E.SetCell(x + dx, y + dy, (uint8)Material::Steam);
                }
            }
        }
    }
}

static void LavaUpdate(Engine& E, int x, int y, const Cell& self) {
    if (E.tryMove(x, y, 0, +1, self)) return;

    bool leftFirst = E.randbit(x, y, app->frames);;
    int da = leftFirst ? -1 : +1, db = -da;

    if (E.GetCell(x + da, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, da, +1, self)) return;
    if (E.GetCell(x + db, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, db, +1, self)) return;

    if (E.GetCell(x + da, y).m == (uint8)Material::Empty && E.tryMove(x, y, da, 0, self)) return;
    if (E.GetCell(x + db, y).m == (uint8)Material::Empty && E.tryMove(x, y, db, 0, self)) return;

    if (E.InRange(x, y - 1) && E.GetCell(x, y - 1).m == (uint8)Material::Empty) {
        if (E.randrange(x, y, app->frames, 100, 3) < 1) {
            E.SetCell(x, y - 1, (uint8)Material::Fire);
        }
    }
}

static void SmokeUpdate(Engine& E, int x, int y, const Cell& self) {
    if (E.tryMove(x, y, 0, -1, self)) return;

    bool leftFirst = E.randbit(x, y, 0);;
    int dxa = leftFirst ? -1 : +1, dxb = -dxa;

    if (E.tryMove(x, y, dxa, -1, self)) return;
    if (E.tryMove(x, y, dxb, -1, self)) return;

    if (E.randrange(x, y, app->frames, 100, 4) < 2) {
        E.SetCell(x, y, (uint8)Material::Empty);
    }
}

static void SteamUpdate(Engine& E, int x, int y, const Cell& self) {
    bool leftFirst = E.randbit(x, y, app->frames);
    int dxa = leftFirst ? -1 : +1, dxb = -dxa;

    if (E.tryMove(x, y, dxa, -1, self)) return;
    if (E.tryMove(x, y, dxb, -1, self)) return;

    if (E.tryMove(x, y, 0, -1, self)) return;

    if (E.randrange(x, y, app->frames, 100, 5) < 2) {
        if (E.randrange(x, y, app->frames, 100, 6) < 90) {
            E.SetCell(x, y, (uint8)Material::Empty);
        }
        else {
            E.SetCell(x, y, (uint8)Material::Water);
        }
    }
}

static void SolidUpdate(Engine&, int, int, const Cell&) { /* inmóvil */ }

static void VineUpdate(Engine& E, int x, int y, const Cell& self) {
    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    //c quema
    for (int i = 0; i < 4; ++i) {
        int nx = x + dirs[i][0], ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        uint8 mat = E.GetCell(nx, ny).m;
        if (mat == (uint8)Material::Fire || mat == (uint8)Material::Lava) {
            E.SetCell(x, y, (uint8)Material::Fire);
            return;
        }
    }

    //c expande
    bool findWater = false;
    for (int i = 0; i < 4; ++i) {
        int nx = x + dirs[i][0], ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
       
        if (E.GetCell(nx, ny).m == (uint8)Material::Water) {
            findWater = true;
        }
    }

    if (!findWater) return;


    uint8 mat = E.GetCell(x, y - 1).m; 

    if (E.InRange(x, y - 1) && (mat == (uint8)Material::Empty || mat == (uint8)Material::Water)) {
        if (E.randrange(x, y, app->frames, 100, 70) < 20) {
            E.SetCell(x, y - 1, (uint8)Material::Vine);
            return;
        }
    }

    if (E.randrange(x, y, app->frames, 100, 71) < 5) {
        const int dir = E.randbit(x, y, app->frames) ? -1 : 1;
        const int nx = x + dir;
        mat = E.GetCell(nx, y).m;
        if (E.InRange(nx, y) && (mat == (uint8)Material::Empty || mat == (uint8)Material::Water)) {
            E.SetCell(nx, y, (uint8)Material::Vine);
            return;
        }
    }
}


void registerDefaultMaterials() {

    //MatProp                                 //NAME         //Color                //Densidad
    g_mat[(uint8)Material::Empty] = { "Empty",          {0,0,0,0,},                 0,              AtlasRectPx{0,0, 32, 32},       nullptr };
    g_mat[(uint8)Material::Sand] = { "Sand",           {217,191,77,255, 1},         3,              AtlasRectPx{32,0, 32, 32},      &SandUpdate };
    g_mat[(uint8)Material::Water] = { "Water",          {51,102,230,200, 1},         1,             AtlasRectPx{64,0, 32, 32},      &WaterUpdate };
    g_mat[(uint8)Material::Stone] = { "Stone",          {128,128,140,255, 1},        255,           AtlasRectPx{96,0, 32, 32},      &SolidUpdate };
    g_mat[(uint8)Material::Wood] = { "Wood",           {142,86,55,255, 1},          255,            AtlasRectPx{128,0, 32, 32},     &WoodUpdate };
    g_mat[(uint8)Material::Fire] = { "Fire",           {255,35,1,255, 5.5f},        255,            AtlasRectPx{160,0, 32, 32},     &FireUpdate };
    g_mat[(uint8)Material::Lava] = { "Lava",           {205,15,1,255, 15.5f},       1,            AtlasRectPx{192,0, 32, 32},     &LavaUpdate };
    g_mat[(uint8)Material::Smoke] = { "Smoke",          {28,13,2,255, 1},            255,           AtlasRectPx{224,0, 32, 32},     &SmokeUpdate };
    g_mat[(uint8)Material::Steam] = { "Steam",          {200,200,200,255, 1},        255,           AtlasRectPx{256,0, 32, 32},     &SteamUpdate };
    g_mat[(uint8)Material::Vine] = { "Vine",            {70,180,70,255, 1},          255,         AtlasRectPx{256,0, 32, 32},     &VineUpdate };

    g_mat[(uint8)Material::NpcCell] = { "NPC",            {220, 40, 200,255},          0,           AtlasRectPx{288,0, 32, 32},     nullptr };
    g_mat[(uint8)Material::NpcSpawnerCell] = { "Spawner", {80, 220, 255,255},          0,           AtlasRectPx{320,0, 32, 32},     nullptr };
    g_mat[(uint8)Material::NpcGoalCell] = { "Goal",       {255, 220, 80,255},          0,           AtlasRectPx{352,0, 32, 32},     nullptr };
    g_mat[(uint8)Material::NpcBonusCell] = { "Bonus",     {20, 20, 20,255},            0,           AtlasRectPx{384,0, 32, 32},     nullptr };
}

bool isVolatile(uint8 m)
{
    return m == (uint8)Material::Fire || m == (uint8)Material::Smoke
        || m == (uint8)Material::Steam;
}
