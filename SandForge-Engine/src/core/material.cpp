#include <array>
#include "material.h"
#include "engine.h"
#include "app/app.h"

static std::array<MatProps, 256> g_mat{};

const MatProps& matProps(uint8 id) { return g_mat[id]; }


static void SandUpdate(Engine& E, int x, int y, const Cell& self) {

    if (E.tryMove(x, y, 0, +1, self)) return; // caer


    if (E.InRange(x, y + 1) && E.GetCell(x, y + 1).m == (uint8)Material::Water && E.trySwap(x, y, 0, +1, self)) return;

    bool leftFirst = E.randbit(x, y, 0);;
    int da = leftFirst ? -1 : +1, db = -da;

    if ((Material)E.GetCell(x + da, y + 1).m == Material::Water && E.trySwap(x, y, da, +1, self)) return;
    if ((Material)E.GetCell(x + db, y + 1).m == Material::Water && E.trySwap(x, y, db, +1, self)) return;

    if (E.tryMove(x, y, da, +1, self)) return;
    E.tryMove(x, y, db, +1, self);
}

static void WaterUpdate(Engine& E, int x, int y, const Cell& self) {
    if (E.tryMove(x, y, 0, +1, self)) return;

    bool leftFirst = E.randbit(x, y , app->frames);
    int da = leftFirst ? -1 : +1, db = -da;

    //Diagonal
    if (E.GetCell(x + da, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, da, +1, self)) return;
    if (E.GetCell(x + db, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, db, +1, self)) return;

    //Horizontal
    if (E.GetCell(x + da, y).m == (uint8)Material::Empty && E.tryMove(x, y, da, 0, self)) return;
    if (E.GetCell(x + db, y).m == (uint8)Material::Empty && E.tryMove(x, y, db, 0, self)) return;

    //Doble Diagonal
    if (E.GetCell(x + da*2, y +2).m == (uint8)Material::Empty && E.GetCell(x + da * 1, y + 1).m == ((uint8)Material::Empty || E.GetCell(x + db * 1, y).m == (uint8)Material::Water) && E.tryMove(x, y, da*2, +2, self)) return;
    if (E.GetCell(x + db*2, y +2).m == (uint8)Material::Empty && E.GetCell(x + db * 1, y + 1).m == ((uint8)Material::Empty || E.GetCell(x + db * 1, y).m == (uint8)Material::Water) &&  E.tryMove(x, y, db*2, +2, self)) return;

    //Semi doble diagonal
    if (E.GetCell(x + da*2, y + 1).m == (uint8)Material::Empty && E.GetCell(x + da * 1, y).m == ((uint8)Material::Empty || E.GetCell(x + db * 1, y).m == (uint8)Material::Water) && E.tryMove(x, y, da*2, +1, self)) return;
    if (E.GetCell(x + db*2, y + 1).m == (uint8)Material::Empty && E.GetCell(x + db * 1, y).m == ((uint8)Material::Empty || E.GetCell(x + db * 1, y).m == (uint8)Material::Water) && E.tryMove(x, y, db*2, +1, self)) return;

    //Doble horizontal
    if (E.GetCell(x + da * 2, y).m == (uint8)Material::Empty && E.GetCell(x + da * 1, y).m == ((uint8)Material::Empty || E.GetCell(x + db * 1, y).m == (uint8)Material::Water) && E.tryMove(x, y, da * 2, 0, self)) return;
    if (E.GetCell(x + db * 2, y).m == (uint8)Material::Empty && E.GetCell(x + db * 1, y).m == ((uint8)Material::Empty || E.GetCell(x + db * 1, y).m == (uint8)Material::Water) && E.tryMove(x, y, db * 2, 0, self)) return;

    
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
        if (E.GetCell(nx, ny).m == (uint8)Material::Fire) {
            E.SetCell(x, y, (uint8)Material::Fire);
            return;
        }
    }
}

static void FireUpdate(Engine& E, int x, int y, const Cell& self) {
    if ((rand() % 100) < 5) {
        E.SetCell(x, y, (uint8)Material::Empty);
        return;
    }

    if (E.InRange(x, y - 1) && E.GetCell(x, y - 1).m == (uint8)Material::Empty) {
        if ((rand() % 100) < 20) {
            E.SetCell(x, y, (uint8)Material::Smoke);
        }
    }

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if ((dx != 0 || dy != 0) && E.InRange(x + dx, y + dy)) {
                if (E.GetCell(x + dx, y + dy).m == (uint8)Material::Wood) {
                    E.SetCell(x, y, (uint8)Material::Fire);
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

    if (E.GetCell(x, y - 1).m == (uint8)Material::Empty) {
        if ((rand() % 100) < 1) {
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

    if ((rand() % 100) < 2) {
        E.SetCell(x, y, (uint8)Material::Empty);
    }
}

static void SteamUpdate(Engine& E, int x, int y, const Cell& self) {
    bool leftFirst = E.randbit(x, y, app->frames);
    int dxa = leftFirst ? -1 : +1, dxb = -dxa;

    if (E.tryMove(x, y, dxa, -1, self)) return;
    if (E.tryMove(x, y, dxb, -1, self)) return;

    if (E.tryMove(x, y, 0, -1, self)) return;

    if ((rand() % 100) < 2) {
        if ((rand() % 100) < 90) {
            E.SetCell(x, y, (uint8)Material::Empty);
        }
        else {
            E.SetCell(x, y, (uint8)Material::Water);
        }
    }
}

static void SolidUpdate(Engine&, int, int, const Cell&) { /* inmóvil */ }


void registerDefaultMaterials() {

    //MatProp                           //NAME      //Color             //Densidad  
    g_mat[(uint8)Material::Empty] = { "Empty",    {0,0,0,0,},           0,                  nullptr };
    g_mat[(uint8)Material::Sand] = { "Sand",     {217,191,77,255, 1},    3,                  &SandUpdate };
    g_mat[(uint8)Material::Water] = { "Water",    {51,102,230,200, 1},    1,                  &WaterUpdate };
    g_mat[(uint8)Material::Stone] = { "Stone",    {128,128,140,255, 1},   255,              &SolidUpdate };
    g_mat[(uint8)Material::Wood] = { "Wood",     {142,86,55,255, 1},     255,                &WoodUpdate };
    g_mat[(uint8)Material::Fire] = { "Fire",     {255,35,1,255, 5.5f},      255,              &FireUpdate };
    g_mat[(uint8)Material::Lava] = { "Lava",     {205,15,1,255, 15.5f},      255,              &LavaUpdate };
    g_mat[(uint8)Material::Smoke] = { "Smoke",    {28,13,2,255, 1},       255,                &SmokeUpdate };
    g_mat[(uint8)Material::Steam] = { "Steam",    {200,200,200,255, 1},   255,               &SteamUpdate };


    g_mat[(uint8)Material::NpcCell] = { "Spawn",    {220, 40, 200,255},           0,                 nullptr };
}

bool isVolatile(uint8 m)
{
    {
        return m == (uint8)Material::Fire || m == (uint8)Material::Smoke
            || m == (uint8)Material::Steam;
    }
}
