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
    if (E.InRange(x, y + 1) && (E.GetCell(x, y + 1).m == (uint8)Material::Oil) && E.trySwap(x, y, 0, +1, self)) return;
    if (E.InRange(x, y + 1) && (E.GetCell(x, y + 1).m == (uint8)Material::Acid) && E.trySwap(x, y, 0, +1, self)) return;

    bool leftFirst = E.randbit(x, y, 0);;
    int da = leftFirst ? -1 : +1, db = -da;

    const uint8 d1 = E.GetCell(x + da, y + 1).m;
    const uint8 d2 = E.GetCell(x + db, y + 1).m;
    if ((d1 == (uint8)Material::Water || d1 == (uint8)Material::Lava || d1 == (uint8)Material::Oil || d1 == (uint8)Material::Acid) && E.trySwap(x, y, da, +1, self)) return;
    if ((d2 == (uint8)Material::Water || d2 == (uint8)Material::Lava || d2 == (uint8)Material::Oil || d2 == (uint8)Material::Acid) && E.trySwap(x, y, db, +1, self)) return;

    if (E.tryMove(x, y, da, +1, self)) return;
    E.tryMove(x, y, db, +1, self);
}

static void SnowUpdate(Engine& E, int x, int y, const Cell& self)
{
    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    for (int i = 0; i < 4; ++i) {
        const int nx = x + dirs[i][0];
        const int ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        if (IsHeatMat(E.GetCell(nx, ny).m)) {
            E.SetCell(x, y, (uint8)Material::Water);
            return;
        }
    }

    if (E.tryMove(x, y, 0, +1, self)) return;

    if (E.InRange(x, y + 1) && E.GetCell(x, y + 1).m == (uint8)Material::Water && E.trySwap(x, y, 0, +1, self)) return;
    if (E.InRange(x, y + 1) && E.GetCell(x, y + 1).m == (uint8)Material::Oil && E.trySwap(x, y, 0, +1, self)) return;
    if (E.InRange(x, y + 1) && E.GetCell(x, y + 1).m == (uint8)Material::Acid && E.trySwap(x, y, 0, +1, self)) return;

    const bool leftFirst = E.randbit(x, y, app->frames);
    const int da = leftFirst ? -1 : +1;
    const int db = -da;

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
    if (E.GetCell(x + da * 2, y + 2).m == (uint8)Material::Empty && passable(x + da, y + 1) && E.tryMove(x, y, da * 2, +2, self)) return;
    if (E.GetCell(x + db * 2, y + 2).m == (uint8)Material::Empty && passable(x + db, y + 1) && E.tryMove(x, y, db * 2, +2, self)) return;

    if (E.GetCell(x + da * 2, y + 1).m == (uint8)Material::Empty && passable(x + da, y) && E.tryMove(x, y, da * 2, +1, self)) return;
    if (E.GetCell(x + db * 2, y + 1).m == (uint8)Material::Empty && passable(x + db, y) && E.tryMove(x, y, db * 2, +1, self)) return;

    //Doble horizontal
    if (E.GetCell(x + da * 2, y).m == (uint8)Material::Empty && passable(x + da, y) && E.tryMove(x, y, da * 2, 0, self)) return;
    if (E.GetCell(x + db * 2, y).m == (uint8)Material::Empty && passable(x + db, y) && E.tryMove(x, y, db * 2, 0, self)) return;

    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    for (int i = 0; i < 4; ++i) {
        int nx = x + dirs[i][0], ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        const uint8 m = E.GetCell(nx, ny).m;
        if (m == (uint8)Material::Lava) {
            E.SetCell(x, y - 1, (uint8)Material::Steam);
            E.SetCell(x, y, (uint8)Material::Stone);
            E.SetCell(nx, ny, (uint8)Material::Stone);
            return;
        }
        if (m == (uint8)Material::HotCoal) {
            E.SetCell(x, y, (uint8)Material::Steam);
            E.SetCell(nx, ny, (uint8)Material::Coal);
            return;
        }
    }
}

static void OilUpdate(Engine& E, int x, int y, const Cell& self)
{
    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    for (int i = 0; i < 4; ++i) {
        const int nx = x + dirs[i][0];
        const int ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        if (IsHeatMat(E.GetCell(nx, ny).m)) {
            E.SetCell(x, y, (uint8)Material::Fire);
            return;
        }
    }

    if (E.tryMove(x, y, 0, +1, self)) return;

    const bool leftFirst = E.randbit(x, y, app->frames);
    const int da = leftFirst ? -1 : +1;
    const int db = -da;

    if (E.GetCell(x + da, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, da, +1, self)) return;
    if (E.GetCell(x + db, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, db, +1, self)) return;

    if (E.randrange(x, y, app->frames, 100, 80) < 60) {
        if (E.GetCell(x + da, y).m == (uint8)Material::Empty && E.tryMove(x, y, da, 0, self)) return;
        if (E.GetCell(x + db, y).m == (uint8)Material::Empty && E.tryMove(x, y, db, 0, self)) return;
    }
}

static void WoodUpdate(Engine& E, int x, int y, const Cell& self) {
    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    for (int i = 0; i < 4; ++i) {
        const int nx = x + dirs[i][0];
        const int ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        const uint8 mat = E.GetCell(nx, ny).m;
        if (mat == (uint8)Material::Fire || mat == (uint8)Material::Lava || mat == (uint8)Material::HotCoal) {
            E.SetCell(x, y, (uint8)Material::Fire);
            return;
        }
    }
}

static void CoalUpdate(Engine& E, int x, int y, const Cell& self)
{
    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    for (int i = 0; i < 4; ++i) {
        const int nx = x + dirs[i][0];
        const int ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        if (IsHeatMat(E.GetCell(nx, ny).m)) {
            E.SetCell(x, y, (uint8)Material::HotCoal);
            return;
        }
    }
}

static void HotCoalUpdate(Engine& E, int x, int y, const Cell& self)
{
    static const int8 dirs4[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    for (int i = 0; i < 4; ++i) {
        const int nx = x + dirs4[i][0];
        const int ny = y + dirs4[i][1];
        if (!E.InRange(nx, ny)) continue;
        if (E.GetCell(nx, ny).m == (uint8)Material::Water) {
            E.SetCell(x, y, (uint8)Material::Coal);
            E.SetCell(nx, ny, (uint8)Material::Steam);
            return;
        }
    }

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!E.InRange(nx, ny)) continue;
            const uint8 m = E.GetCell(nx, ny).m;
            if (m == (uint8)Material::Empty) {
                if (E.randrange(nx, ny, app->frames, 100, 81) < 10) {
                    E.SetCell(nx, ny, (uint8)Material::Fire);
                }
            }
            else if (m == (uint8)Material::Coal && E.randrange(nx, ny, app->frames, 100, 82) < 18) {
                E.SetCell(nx, ny, (uint8)Material::HotCoal);
            }
            else if (m == (uint8)Material::Dynamite) {
                ExplodeIntoFire(E, nx, ny, 5);
                return;
            }
            else if (m == (uint8)Material::FlammableGas) {
                ExplodeIntoFire(E, nx, ny, 2);
                return;
            }
            else if (IsFlammableMat(m) && E.randrange(nx, ny, app->frames, 100, 83) < 18) {
                E.SetCell(nx, ny, (uint8)Material::Fire);
            }
        }
    }

    if (E.randrange(x, y, app->frames, 100, 84) < 2) {
        E.SetCell(x, y, (uint8)Material::Coal);
    }
}

static void FireUpdate(Engine& E, int x, int y, const Cell& self)
{
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
            if (dx == 0 && dy == 0) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!E.InRange(nx, ny)) continue;
            if (E.GetCell(nx, ny).m == (uint8)Material::Water) {
                E.SetCell(x, y, (uint8)Material::Steam);
                E.SetCell(nx, ny, (uint8)Material::Steam);
                return;
            }
        }
    }

    TryIgniteNeighbours(E, x, y);
}

static void LavaUpdate(Engine& E, int x, int y, const Cell& self)
{
    TryIgniteNeighbours(E, x, y);

    if (E.tryMove(x, y, 0, +1, self)) return;

    const bool leftFirst = E.randbit(x, y, app->frames);
    const int da = leftFirst ? -1 : +1;
    const int db = -da;

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

static void SmokeUpdate(Engine& E, int x, int y, const Cell& self)
{
    if (E.tryMove(x, y, 0, -1, self)) return;

    const bool leftFirst = E.randbit(x, y, 0);
    const int dxa = leftFirst ? -1 : +1;
    const int dxb = -dxa;

    if (E.tryMove(x, y, dxa, -1, self)) return;
    if (E.tryMove(x, y, dxb, -1, self)) return;

    if (E.randrange(x, y, app->frames, 100, 4) < 2) {
        E.SetCell(x, y, (uint8)Material::Empty);
    }
}

static void SteamUpdate(Engine& E, int x, int y, const Cell& self)
{
    const bool leftFirst = E.randbit(x, y, app->frames);
    const int dxa = leftFirst ? -1 : +1;
    const int dxb = -dxa;

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

static void FlammableGasUpdate(Engine& E, int x, int y, const Cell& self)
{
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!E.InRange(nx, ny)) continue;
            if (IsHeatMat(E.GetCell(nx, ny).m)) {
                ExplodeIntoFire(E, x, y, 2);
                return;
            }
        }
    }

    if (E.tryMove(x, y, 0, -1, self)) return;

    const bool leftFirst = E.randbit(x, y, app->frames);
    const int dxa = leftFirst ? -1 : +1;
    const int dxb = -dxa;

    if (E.tryMove(x, y, dxa, -1, self)) return;
    if (E.tryMove(x, y, dxb, -1, self)) return;
    if (E.tryMove(x, y, dxa, 0, self)) return;
    if (E.tryMove(x, y, dxb, 0, self)) return;

    if (E.randrange(x, y, app->frames, 100, 85) < 2) {
        E.SetCell(x, y, (uint8)Material::Empty);
    }
}

static void AcidUpdate(Engine& E, int x, int y, const Cell& self)
{
    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    const int start = E.randrange(x, y, app->frames, 4, 90);
    for (int k = 0; k < 4; ++k) {
        const int i = (start + k) & 3;
        const int nx = x + dirs[i][0];
        const int ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        const uint8 m = E.GetCell(nx, ny).m;
        if (!IsAcidTarget(m)) continue;

        if (E.randrange(nx, ny, app->frames, 100, 91) < 35) {
            E.SetCell(nx, ny, (uint8)Material::Empty);
            if (E.randrange(x, y, app->frames, 100, 92) < 22) {
                E.SetCell(x, y, (uint8)Material::Empty);
            }
            return;
        }
    }

    if (E.tryMove(x, y, 0, +1, self)) return;

    const bool leftFirst = E.randbit(x, y, app->frames);
    const int da = leftFirst ? -1 : +1;
    const int db = -da;

    if (E.GetCell(x + da, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, da, +1, self)) return;
    if (E.GetCell(x + db, y + 1).m == (uint8)Material::Empty && E.tryMove(x, y, db, +1, self)) return;

    if (E.randrange(x, y, app->frames, 100, 93) < 55) {
        if (E.GetCell(x + da, y).m == (uint8)Material::Empty && E.tryMove(x, y, da, 0, self)) return;
        if (E.GetCell(x + db, y).m == (uint8)Material::Empty && E.tryMove(x, y, db, 0, self)) return;
    }

    if (E.randrange(x, y, app->frames, 100, 94) < 1) {
        E.SetCell(x, y, (uint8)Material::Empty);
    }
}

static void SolidUpdate(Engine&, int, int, const Cell&) { }
static void FragilePlatformUpdate(Engine&, int, int, const Cell&) { }

static void DynamiteUpdate(Engine& E, int x, int y, const Cell& self)
{
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!E.InRange(nx, ny)) continue;
            if (IsHeatMat(E.GetCell(nx, ny).m)) {
                ExplodeIntoFire(E, x, y, 10);
                return;
            }
        }
    }
}

static void VineUpdate(Engine& E, int x, int y, const Cell& self)
{
    static const int8 dirs[4][2] = {
                    {0,-1},
            {-1, 0},       {1, 0},
                    {0, 1}
    };

    //c quema
    for (int i = 0; i < 4; ++i) {
        const int nx = x + dirs[i][0];
        const int ny = y + dirs[i][1];
        if (!E.InRange(nx, ny)) continue;
        const uint8 mat = E.GetCell(nx, ny).m;
        if (mat == (uint8)Material::Fire || mat == (uint8)Material::Lava || mat == (uint8)Material::HotCoal) {
            E.SetCell(x, y, (uint8)Material::Fire);
            return;
        }
    }

    //c expande
    bool findWater = false;
    for (int i = 0; i < 4; ++i) {
        const int nx = x + dirs[i][0];
        const int ny = y + dirs[i][1];
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

void registerDefaultMaterials()
{
    //MatProp                                 //NAME            //Color                   //Densidad          //Atlas                         //Rules
    g_mat[(uint8)Material::Empty] = {       "Empty",            {0,0,0,0,},                 0,                  AtlasRectPx{0,0, 32, 32},       nullptr };
    g_mat[(uint8)Material::Sand] = {        "Sand",             {217,191,77,255, 1},        3,                  AtlasRectPx{32,0, 32, 32},      &SandUpdate };
    g_mat[(uint8)Material::Water] = {       "Water",            {51,102,230,200, 1},        1,                  AtlasRectPx{64,0, 32, 32},      &WaterUpdate };
    g_mat[(uint8)Material::Stone] = {       "Stone",            {128,128,140,255, 1},       255,                AtlasRectPx{96,0, 32, 32},      &SolidUpdate };
    g_mat[(uint8)Material::Wood] = {        "Wood",             {142,86,55,255, 1},         255,                AtlasRectPx{128,0, 32, 32},     &WoodUpdate };
    g_mat[(uint8)Material::Fire] = {        "Fire",             {255,35,1,255, 5.5f},       255,                AtlasRectPx{160,0, 32, 32},     &FireUpdate };
    g_mat[(uint8)Material::Lava] = {        "Lava",             {205,15,1,255, 15.5f},      1,                  AtlasRectPx{192,0, 32, 32},     &LavaUpdate };
    g_mat[(uint8)Material::Smoke] = {       "Smoke",            {28,13,2,255, 1},           255,                AtlasRectPx{224,0, 32, 32},     &SmokeUpdate };
    g_mat[(uint8)Material::Steam] = {       "Steam",            {200,200,200,255, 1},       255,                AtlasRectPx{256,0, 32, 32},     &SteamUpdate };
    g_mat[(uint8)Material::Vine] = {        "Vine",             {70,180,70,255, 1},         255,                AtlasRectPx{0, 32, 32, 32},     &VineUpdate };
    g_mat[(uint8)Material::Snow] = {        "Snow",             {235,240,255,255, 1},       2,                  AtlasRectPx{32,32, 32, 32},     &SnowUpdate };
    g_mat[(uint8)Material::Oil] = {         "Oil",              {95,70,30,220, 1},          1,                  AtlasRectPx{64,32, 32, 32},     &OilUpdate };
    g_mat[(uint8)Material::Coal] = {        "Coal",             {45,45,52,255, 1},          255,                AtlasRectPx{96,32, 32, 32},     &CoalUpdate };
    g_mat[(uint8)Material::HotCoal] = {     "HotCoal",          {255,110,35,255, 3.0f},     255,                AtlasRectPx{128,32, 32, 32},    &HotCoalUpdate };
    g_mat[(uint8)Material::FlammableGas] = {"Gas",              {255,220,150,120, 1.6f},    1,                  AtlasRectPx{160,32, 32, 32},    &FlammableGasUpdate };
    g_mat[(uint8)Material::FragilePlatform] = {"Fragile",       {180,150,110,255, 1},       255,                AtlasRectPx{192,32, 32, 32},    &FragilePlatformUpdate };
    g_mat[(uint8)Material::Acid] = {        "Acid",             {90,255,90,220, 1},         2,                  AtlasRectPx{224,32, 32, 32},    &AcidUpdate };
    g_mat[(uint8)Material::Dynamite] = {    "Dynamite",         {220,50,50,255, 1},         255,                AtlasRectPx{256,32, 32, 32},    &DynamiteUpdate };

    g_mat[(uint8)Material::NpcCell] = {     "NPC",              {220, 40, 200,255},         0,                  AtlasRectPx{288,0, 32, 32},     nullptr };
    g_mat[(uint8)Material::NpcSpawnerCell] = {"Spawner",        {80, 220, 255,255},         0,                  AtlasRectPx{320,0, 32, 32},     nullptr };
    g_mat[(uint8)Material::NpcGoalCell] = { "Goal",             {255, 220, 80,255},         0,                  AtlasRectPx{352,0, 32, 32},     nullptr };
    g_mat[(uint8)Material::NpcBonusCell] = {"Bonus",            {20, 20, 20,255},           0,                  AtlasRectPx{384,0, 32, 32},     nullptr };
    g_mat[(uint8)Material::PlayerCell] = {  "Player",           {255, 150, 215,255},        0,                  AtlasRectPx{288,0, 32, 32},     nullptr };
    g_mat[(uint8)Material::PlayerTriggerCell] = {"PlayerTrigger", {255, 200, 120,255},      0,                  AtlasRectPx{384,0, 32, 32},     nullptr };
}

bool isVolatile(uint8 m)
{
    return m == (uint8)Material::Fire
        || m == (uint8)Material::Smoke
        || m == (uint8)Material::Steam
        || m == (uint8)Material::HotCoal
        || m == (uint8)Material::FlammableGas;
}

bool IsHeatMat(uint8 m)
{
    return m == (uint8)Material::Fire || m == (uint8)Material::Lava || m == (uint8)Material::HotCoal;
}

bool IsFlammableMat(uint8 m)
{
    return m == (uint8)Material::Wood
        || m == (uint8)Material::Vine
        || m == (uint8)Material::Oil
        || m == (uint8)Material::FragilePlatform;
}

bool IsAcidTarget(uint8 m)
{
    return m == (uint8)Material::Stone
        || m == (uint8)Material::Wood
        || m == (uint8)Material::Vine
        || m == (uint8)Material::Coal
        || m == (uint8)Material::HotCoal
        || m == (uint8)Material::FragilePlatform
        || m == (uint8)Material::Dynamite
        || m == (uint8)Material::Snow;
}

void ExplodeIntoFire(Engine& E, int cx, int cy, int radius)
{
    const int r2 = radius * radius;
    for (int y = cy - radius; y <= cy + radius; ++y) {
        for (int x = cx - radius; x <= cx + radius; ++x) {
            if (!E.InRange(x, y)) continue;
            const int dx = x - cx;
            const int dy = y - cy;
            if (dx * dx + dy * dy > r2) continue;
            E.SetCell(x, y, (uint8)Material::Fire);
        }
    }
}

void TryIgniteNeighbours(Engine& E, int x, int y)
{
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const int nx = x + dx;
            const int ny = y + dy;
            if (!E.InRange(nx, ny)) continue;
            const uint8 m = E.GetCell(nx, ny).m;

            if (m == (uint8)Material::Snow) {
                E.SetCell(nx, ny, (uint8)Material::Water);
                continue;
            }
            if (m == (uint8)Material::Coal) {
                E.SetCell(nx, ny, (uint8)Material::HotCoal);
                continue;
            }
            if (m == (uint8)Material::Dynamite) {
                ExplodeIntoFire(E, nx, ny, 5);
                continue;
            }
            if (m == (uint8)Material::FlammableGas) {
                ExplodeIntoFire(E, nx, ny, 2);
                continue;
            }
            if (IsFlammableMat(m)) {
                E.SetCell(nx, ny, (uint8)Material::Fire);
            }
        }
    }
}
