#include "player_system.h"

#include "worldsim.h"
#include "engine.h"
#include "app/app.h"
#include "render/renderer.h"
#include "render/atlas.h"
#include "input.h"
#include "ui/ui.h"

#include <algorithm>
#include <cmath>

static Player MakeDefaultPlayer(const Texture2D* tex, const SpriteAnimLibrary* anims, int x, int y)
{
    Player p{};

    p.x = x;
    p.y = y;
    p.w = 12;
    p.h = 12;

    const int padX = 3;
    const int padTop = 5;
    const int padBottom = 0;

    p.hbOffX = std::clamp(padX, 0, (int)std::fmax(0, p.w - 1));
    p.hbOffY = std::clamp(padTop, 0, (int)std::fmax(0, p.h - 1));
    p.hbW = std::fmax(1, p.w - 2 * padX);
    p.hbH = std::fmax(1, p.h - padTop - padBottom);

    p.dir = 1;
    p.alive = true;
    p.jumpCellsLeft = 0;
    p.heldMaterial = Material::Empty;
    p.placeCooldown = 0.0f;
    p.jumpBuffer = 0.0f;
    p.coyoteTime = 0.0f;

    p.sprite.tex = tex;
    p.sprite.color = RGBAu32(255, 165, 220, 255);

    p.anim.SetLibrary(anims);
    p.anim.Play("idle", true);
    p.anim.ApplyTo(p.sprite, false);

    return p;
}

static bool RectsOverlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < (bx + bw) && (ax + aw) > bx && ay < (by + bh) && (ay + ah) > by;
}

static bool CircleIntersectsRect(int cx, int cy, int radius, int rx, int ry, int rw, int rh)
{
    const int nearestX = std::clamp(cx, rx, rx + rw - 1);
    const int nearestY = std::clamp(cy, ry, ry + rh - 1);
    const int dx = cx - nearestX;
    const int dy = cy - nearestY;
    return dx * dx + dy * dy <= radius * radius;
}

static bool IsPlayerPassableMat(uint8 m)
{
    return m == (uint8)Material::Empty
        || m == (uint8)Material::Water
        || m == (uint8)Material::Oil
        || m == (uint8)Material::Acid
        || m == (uint8)Material::Smoke
        || m == (uint8)Material::Steam
        || m == (uint8)Material::FlammableGas
        || m == (uint8)Material::Vine;
}

static bool IsPlayerUsableMaterial(Material m)
{
    return m >= Material::Sand && m <= Material::Dynamite;
}

bool PlayerSystem::Awake(const WorldSim&)
{
    return true;
}

bool PlayerSystem::Start()
{
    playerTex.Load(SPRITE_DIR "/KoboldMiner.png");
    triggerIconsTex.Load(SPRITE_DIR "/materialAtlas.png");

    playerAnims.Clear();
    {
        auto& idle = playerAnims.Add("idle");
        idle.defaultTex = &playerTex;
        idle.fps = 6.0f;
        idle.loop = AnimLoopMode::Loop;
        idle.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 0,0,12,12 }, 0.1f));
        idle.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 12,0,12,12 }, 0.1f));
        idle.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 24,0,12,12 }, 0.1f));
        idle.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 12,0,12,12 }, 0.1f));

        auto& walk = playerAnims.Add("walk");
        walk.defaultTex = &playerTex;
        walk.fps = 6.0f;
        walk.loop = AnimLoopMode::Loop;
        walk.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 0,12,12,12 }, 0.1f));
        walk.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 12,12,12,12 }, 0.1f));
        walk.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 24,12,12,12 }, 0.1f));
        walk.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 36,12,12,12 }, 0.1f));
        walk.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 48,12,12,12 }, 0.1f));

        auto& fall = playerAnims.Add("fall");
        fall.defaultTex = &playerTex;
        fall.fps = 6.0f;
        fall.loop = AnimLoopMode::PingPong;
        fall.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 0,24,12,12 }, 0.1f));
        fall.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 12,24,12,12 }, 0.1f));
        fall.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 24,24,12,12 }, 0.1f));

        auto& climb = playerAnims.Add("climb");
        climb.defaultTex = &playerTex;
        climb.fps = 6.0f;
        climb.loop = AnimLoopMode::Loop;
        climb.frames.push_back(AnimFramePx(&playerTex, AtlasRectPx{ 0,0,12,12 }, 0.1f));
    }

    return true;
}

void PlayerSystem::Clear(const WorldSim&)
{
    player = Player{};
    hasPlayer = false;
    playerTriggers.clear();
    moveAcc = 0.0f;
}

Player& PlayerSystem::AddPlayer(WorldSim& world, int x, int y, Material startMaterial)
{
    player = MakeDefaultPlayer(&playerTex, &playerAnims, x, y);
    player.heldMaterial = IsPlayerUsableMaterial(startMaterial) ? startMaterial : Material::Empty;
    hasPlayer = true;

    world.MarkChunksInRect(x, y, player.w, player.h);
    return player;
}

PlayerMaterialTrigger& PlayerSystem::AddPlayerMaterialTrigger(WorldSim& world, int x, int y, Material material)
{
    PlayerMaterialTrigger trigger{};
    trigger.x = x;
    trigger.y = y;
    trigger.w = 12;
    trigger.h = 12;
    trigger.material = IsPlayerUsableMaterial(material) ? material : Material::Sand;
    trigger.sprite.tex = &triggerIconsTex;
    trigger.sprite.color = RGBAu32(255, 255, 255, 240);

    playerTriggers.push_back(trigger);
    world.MarkChunksInRect(x, y, trigger.w, trigger.h);

    return playerTriggers.back();
}

bool PlayerSystem::EraseEntitiesInCircle(WorldSim& world, int cx, int cy, int radius,
    bool erasePlayer, bool eraseTriggers)
{
    bool removedAny = false;

    if (erasePlayer && hasPlayer && CircleIntersectsRect(cx, cy, radius, player.x, player.y, player.w, player.h)) {
        hasPlayer = false;
        player = Player{};
        removedAny = true;
    }

    if (eraseTriggers) {
        const size_t oldSize = playerTriggers.size();
        playerTriggers.erase(std::remove_if(playerTriggers.begin(), playerTriggers.end(), [&](const PlayerMaterialTrigger& trigger) {
            return CircleIntersectsRect(cx, cy, radius, trigger.x, trigger.y, trigger.w, trigger.h);
        }), playerTriggers.end());
        if (playerTriggers.size() != oldSize) {
            removedAny = true;
        }
    }

    if (removedAny) {
        world.MarkChunksInRect(cx - radius, cy - radius, radius * 2 + 1, radius * 2 + 1);
    }

    return removedAny;
}

bool PlayerSystem::RectFreeOnBack(const WorldSim& world, const std::vector<int>& occ,
    int x, int y, int w, int h, int ignoreId) const
{
    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            const int gx = x + xx;
            const int gy = y + yy;
            if (!world.InRange(gx, gy)) return false;

            const int wi = world.LinearIndex(gx, gy);
            if (!IsPlayerPassableMat(world.BackMatAtIndex(wi))) return false;

            const int occId = occ.empty() ? 0 : occ[wi];
            if (occId != 0 && occId != ignoreId) return false;
        }
    }

    return true;
}

bool PlayerSystem::IsInVine(const WorldSim& world, int x, int y, int w, int h) const
{
    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            const int gx = x + xx;
            const int gy = y + yy;
            if (!world.InRange(gx, gy)) continue;
            if (world.BackMatAtIndex(world.LinearIndex(gx, gy)) == (uint8)Material::Vine) {
                return true;
            }
        }
    }

    return false;
}

bool PlayerSystem::HasVineBelow(const WorldSim& world, int x, int y, int w, int h) const
{
    const int gy = y + h;
    for (int xx = 0; xx < w; ++xx) {
        const int gx = x + xx;
        if (!world.InRange(gx, gy)) continue;
        if (world.BackMatAtIndex(world.LinearIndex(gx, gy)) == (uint8)Material::Vine) {
            return true;
        }
    }

    return false;
}

bool PlayerSystem::GetPlaceTargetCell(int mouseWorldX, int mouseWorldY, int& outX, int& outY) const
{
    if (!hasPlayer || !player.alive) return false;

    const float originX = player.x + player.w * 0.5f;
    const float originY = player.y + player.hbOffY + player.hbH * 0.5f;

    float dx = ((float)mouseWorldX + 0.5f) - originX;
    float dy = ((float)mouseWorldY + 0.5f) - originY;

    const float maxDistance = 20.0f;
    const float distSq = dx * dx + dy * dy;
    if (distSq > maxDistance * maxDistance) {
        const float dist = std::sqrt(distSq);
        if (dist > 0.0001f) {
            const float k = maxDistance / dist;
            dx *= k;
            dy *= k;
        }
    }

    outX = (int)std::floor(originX + dx);
    outY = (int)std::floor(originY + dy);
    return true;
}

bool PlayerSystem::TryTouchMaterialTrigger(Player& p)
{
    if (!hasPlayer || !p.alive) return false;

    const int hx = p.x + p.hbOffX;
    const int hy = p.y + p.hbOffY;
    for (const PlayerMaterialTrigger& trigger : playerTriggers) {
        if (!RectsOverlap(hx, hy, p.hbW, p.hbH, trigger.x, trigger.y, trigger.w, trigger.h)) continue;
        p.heldMaterial = trigger.material;
        return true;
    }

    return false;
}

bool PlayerSystem::TryPlaceMaterial(WorldSim& world, const std::vector<int>& occ, Player& p)
{
    if (!IsPlayerUsableMaterial(p.heldMaterial)) return false;
    if (!app || !app->input || !app->engine) return false;

    int centerX = 0;
    int centerY = 0;
    if (!app->engine->GetPlayerPlaceTargetCellFromMouse((int)app->input->MouseX(), (int)app->input->MouseY(), centerX, centerY)) {
        return false;
    }

    centerX = std::clamp(centerX, 0, world.GridW() - 1);
    centerY = std::clamp(centerY, 0, world.GridH() - 1);

    const int radius = 2;
    const int r2 = radius * radius;

    int placedCount = 0;

    const int xmin = std::fmax(0, centerX - radius);
    const int xmax = std::fmin(world.GridW() - 1, centerX + radius);
    const int ymin = std::fmax(0, centerY - radius);
    const int ymax = std::fmin(world.GridH() - 1, centerY + radius);

    for (int y = ymin; y <= ymax; ++y) {
        for (int x = xmin; x <= xmax; ++x) {
            const int dx = x - centerX;
            const int dy = y - centerY;
            if (dx * dx + dy * dy > r2) continue;

            const int wi = world.LinearIndex(x, y);
            if (world.BackMatAtIndex(wi) != (uint8)Material::Empty) continue;
            if (!occ.empty() && occ[wi] != 0) continue;

            world.SetCell(x, y, (uint8)p.heldMaterial);
            ++placedCount;
        }
    }

    if (placedCount > 0 && app && app->engine) {
        app->engine->AddMaterialUse(placedCount);
        return true;
    }

    return false;
}

void PlayerSystem::Move(WorldSim& world, const std::vector<int>& occ, float fixedTimeStep)
{
    if (!hasPlayer || !player.alive || !app || !app->input) return;

    const bool moveLeft = app->input->KeyRepeat(GLFW_KEY_A);
    const bool moveRight = app->input->KeyRepeat(GLFW_KEY_D);
    const bool moveDown = app->input->KeyRepeat(GLFW_KEY_S);
    const bool jumpPressed = app->input->KeyDown(GLFW_KEY_W);
    const bool jumpHeld = app->input->KeyRepeat(GLFW_KEY_W);
    const bool placeHeld = app->input->KeyRepeat(GLFW_KEY_SPACE);

    if (moveLeft != moveRight) {
        player.dir = moveLeft ? -1 : 1;
    }

    player.placeCooldown = std::fmax(0.0f, player.placeCooldown - fixedTimeStep);
    player.jumpBuffer = std::fmax(0.0f, player.jumpBuffer - fixedTimeStep);
    player.coyoteTime = std::fmax(0.0f, player.coyoteTime - fixedTimeStep);

    if (jumpPressed) {
        player.jumpBuffer = 0.12f;
    }

    TryTouchMaterialTrigger(player);

    int hbX = player.x + player.hbOffX;
    int hbY = player.y + player.hbOffY;
    const int id = -1;

    const bool inVine = IsInVine(world, hbX, hbY, player.hbW, player.hbH);
    const bool grounded = !RectFreeOnBack(world, occ, hbX, hbY + 1, player.hbW, player.hbH, id)
        || HasVineBelow(world, hbX, hbY, player.hbW, player.hbH);

    if (grounded || inVine) {
        player.coyoteTime = 0.10f;
    }

    if (player.jumpBuffer > 0.0f && (player.coyoteTime > 0.0f || inVine)) {
        player.jumpCellsLeft = 4;
        player.jumpBuffer = 0.0f;
        if (!inVine) {
            player.coyoteTime = 0.0f;
        }
    }

    if (placeHeld && player.placeCooldown <= 0.0f) {
        if (TryPlaceMaterial(world, occ, player)) {
            player.placeCooldown = 0.025f;
        }
    }

    moveAcc += fixedTimeStep;
    const float moveInterval = 1.0f / std::fmax(0.001f, cellsPerSec);
    if (moveAcc < moveInterval) return;
    moveAcc -= moveInterval;

    int moveDir = 0;
    if (moveLeft != moveRight) {
        moveDir = moveLeft ? -1 : 1;
        player.dir = moveDir;
    }

    bool moved = false;
    bool climbedVine = false;
    bool steppedDown = false;
    bool fell = false;
    const int kMaxStep = 2;

    hbX = player.x + player.hbOffX;
    hbY = player.y + player.hbOffY;

    if (inVine) {
        if (jumpHeld && RectFreeOnBack(world, occ, hbX, hbY - 1, player.hbW, player.hbH, id)) {
            player.y -= 1;
            moved = true;
            climbedVine = true;
        }
        else if (moveDown && RectFreeOnBack(world, occ, hbX, hbY + 1, player.hbW, player.hbH, id)) {
            player.y += 1;
            moved = true;
            climbedVine = true;
        }
    }
    else if (player.jumpCellsLeft > 0) {
        if (RectFreeOnBack(world, occ, hbX, hbY - 1, player.hbW, player.hbH, id)) {
            player.y -= 1;
            player.jumpCellsLeft -= 1;
            moved = true;
        }
        else {
            player.jumpCellsLeft = 0;
        }
    }
    else if (RectFreeOnBack(world, occ, hbX, hbY + 1, player.hbW, player.hbH, id)) {
        player.y += 1;
        moved = true;

        const int nextHbX = player.x + player.hbOffX;
        const int nextHbY = player.y + player.hbOffY;
        if (RectFreeOnBack(world, occ, nextHbX, nextHbY + 1, player.hbW, player.hbH, id)) {
            fell = true;
        }
    }

    hbX = player.x + player.hbOffX;
    hbY = player.y + player.hbOffY;

    if (moveDir != 0) {
        const int nx = player.x + moveDir;

        if (RectFreeOnBack(world, occ, nx + player.hbOffX, hbY, player.hbW, player.hbH, id)) {
            player.x = nx;
            moved = true;
        }
        else {
            auto solid = [&](int gx, int gy) {
                return world.InRange(gx, gy) && !IsPlayerPassableMat(world.BackMatAtIndex(world.LinearIndex(gx, gy)));
            };

            bool climbed = false;
            for (int step = 1; step <= kMaxStep; ++step) {
                if (!RectFreeOnBack(world, occ, nx + player.hbOffX, (player.y - step) + player.hbOffY, player.hbW, player.hbH, id)) {
                    continue;
                }

                bool support = false;
                const int supY = (player.y - step) + player.hbOffY + player.hbH;
                for (int xx = 0; xx < player.hbW; ++xx) {
                    support |= solid((nx + player.hbOffX) + xx, supY);
                }
                if (!support) continue;

                player.y -= step;
                player.x = nx;
                climbed = true;
                moved = true;
                break;
            }

            if (!climbed && !inVine && RectFreeOnBack(world, occ, nx + player.hbOffX, hbY + 1, player.hbW, player.hbH, id)) {
                player.x = nx;
                player.y += 1;
                moved = true;
                steppedDown = true;
            }
        }
    }

    TryTouchMaterialTrigger(player);

    if (climbedVine) player.anim.Play("climb", false);
    else if (steppedDown) player.anim.Play("walk", false);
    else if (fell) player.anim.Play("fall", false);
    else if (moveDir != 0 && moved) player.anim.Play("walk", false);
    else player.anim.Play("idle", false);
}

void PlayerSystem::Animate(Engine& engine, float dt)
{
    App* app = engine.app;
    if (!app || !app->renderer) return;

    float sx = 0.0f;
    float sy = 0.0f;
    float sw = 0.0f;
    float sh = 0.0f;

    if (hasPlayer && player.alive) {
        player.anim.Update(dt);
        player.anim.ApplyTo(player.sprite, player.dir < 0);
        player.sprite.color = RGBAu32(255, 165, 220, 255);

        if (engine.WorldRectToScreen((float)player.x, (float)player.y, (float)player.w, (float)player.h,
            app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh)) {

            player.sprite.x = std::floor(sx);
            player.sprite.y = std::floor(sy);
            player.sprite.w = std::floor(sw);
            player.sprite.h = std::floor(sh);
            player.sprite.layer = RenderLayer::WORLD;
            player.sprite.sort = 5;
            app->renderer->Queue(player.sprite);

            if (triggerIconsTex.id != 0 && IsPlayerUsableMaterial(player.heldMaterial)) {
                Sprite icon{};
                icon.tex = &triggerIconsTex;
                icon.x = std::floor(sx + sw * 0.25f);
                icon.y = std::floor(sy - sh * 0.45f);
                icon.w = std::floor(sw * 0.5f);
                icon.h = std::floor(sh * 0.5f);
                icon.layer = RenderLayer::WORLD;
                icon.sort = 6;
                icon.color = RGBAu32(255, 255, 255, 235);

                const UVRect uv = UVFromPx(triggerIconsTex, matProps((uint8)player.heldMaterial).rect);
                icon.u0 = uv.u0;
                icon.v0 = uv.v0;
                icon.u1 = uv.u1;
                icon.v1 = uv.v1;
                app->renderer->Queue(icon);
            }
        }
    }

    for (auto& trigger : playerTriggers) {
        if (triggerIconsTex.id == 0) continue;
        if (!IsPlayerUsableMaterial(trigger.material)) continue;

        if (!engine.WorldRectToScreen((float)trigger.x, (float)trigger.y, (float)trigger.w, (float)trigger.h,
            app->framebufferSize.x, app->framebufferSize.y, sx, sy, sw, sh)) {
            continue;
        }

        trigger.sprite.x = std::floor(sx);
        trigger.sprite.y = std::floor(sy);
        trigger.sprite.w = std::floor(sw);
        trigger.sprite.h = std::floor(sh);
        trigger.sprite.layer = RenderLayer::WORLD;
        trigger.sprite.sort = -11;

        const UVRect uv = UVFromPx(triggerIconsTex, matProps((uint8)trigger.material).rect);
        trigger.sprite.u0 = uv.u0;
        trigger.sprite.v0 = uv.v0;
        trigger.sprite.u1 = uv.u1;
        trigger.sprite.v1 = uv.v1;

        app->renderer->Queue(trigger.sprite);
    }
}
