#include "engine.h"
#include "app/app.h"
#include <algorithm>
#include <cmath>

Engine::Engine(App* app, bool start_enabled) : Module(app, start_enabled) {}
Engine::~Engine() = default;

bool Engine::Awake()
{
    if (!world.Awake(app)) return false;
    npcs.Awake(world);
    paint.Clear();
    return true;
}

bool Engine::Start()
{
    return npcs.Start();
}

bool Engine::PreUpdate() { return true; }

bool Engine::Update(float dt)
{
    world.StepAccumulator() += dt;

    const float fixed = world.FixedTimeStep();
    while (world.StepAccumulator() >= fixed && (!paused || stepOnce)) {
        world.CopyFrontToBack();

        npcs.RebuildOcc(world);
        world.Step(*this, parity);
        npcs.MoveNPCs(world, fixed);
        npcs.RebuildOcc(world);

        world.SwapBuffers();

        world.StepAccumulator() -= fixed;
        parity ^= 1;

        if (paused) { stepOnce = false; break; }
    }

    npcs.AnimateNPCs(*this, dt);

    if (paused) world.StepAccumulator() = 0.0f;

    return true;
}

bool Engine::PostUpdate() { return true; }
bool Engine::CleanUp() { return true; }

void Engine::ClearWorld(uint8 fill)
{
    world.Clear(fill);
    npcs.Clear(world);
    paint.Clear();

    parity = 0;
    stepOnce = false;

    npcs.RebuildOcc(world);
}

bool Engine::tryMove(int x0, int y0, int x1, int y1, const Cell& c)
{
    return world.TryMove(x0, y0, x1, y1, c, npcs.Occ());
}

bool Engine::trySwap(int x0, int y0, int x1, int y1, const Cell& c)
{
    return world.TrySwap(x0, y0, x1, y1, c, npcs.Occ());
}

void Engine::SetCell(int x, int y, uint8 m)
{
    world.SetCell(x, y, m);
}

void Engine::Paint(int screenX, int screenY, Material m, int r)
{
    paint.Paint(*this, world, npcs, screenX, screenY, m, r);
}

void Engine::StopPaint()
{
    paint.StopPaint(*this);
}

bool Engine::randbit(int x, int y, int parity)
{
    uint32_t h = (uint32_t)(x * 374761393u) ^ (uint32_t)(y * 668265263u) ^ (uint32_t)(parity * 0x9E3779B9u);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    return (h & 1u) != 0u;
}

bool Engine::WorldRectToScreen(float x, float y, float w, float h,
    int vw, int vh,
    float& sx, float& sy, float& sw, float& sh)
{
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
    outX = std::clamp(outX, 0, world.GridW() - 1);
    outY = std::clamp(outY, 0, world.GridH() - 1);

    return true;
}
