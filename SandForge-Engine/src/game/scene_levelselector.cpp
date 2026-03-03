#include "scene_levelselector.h"
#include "scene_manager.h"
#include "app/app.h"
#include "core/input.h"
#include "ui/ui.h"
#include <cmath>
#include <cstdio>

// ------------------------------------------------------------
//  Scene_LevelSelector
// ------------------------------------------------------------

void Scene_LevelSelector::Update(float dt)
{
    (void)dt;

    // ESC -> volver al menu
    if (app->input->KeyDown(GLFW_KEY_ESCAPE))
        mgr->Request(SCENE_MAINMENU);
}

void Scene_LevelSelector::DrawUI(int&, Material&)
{
    UI* ui = app->ui;

    const float vw = (float)app->framebufferSize.x;
    const float vh = (float)app->framebufferSize.y;
    if (vw <= 0.0f || vh <= 0.0f) return;

    // Escalado simple en base a 1280x720
    float uiScale = std::fmin(vw / 1280.0f, vh / 720.0f);
    if (uiScale < 0.85f) uiScale = 0.85f;
    if (uiScale > 1.25f) uiScale = 1.25f;
    auto U = [&](float v) { return v * uiScale; };

    // --------------------------------------------------------
    // Fondo + barra superior
    // --------------------------------------------------------
    const uint32 bg  = RGBAu32(18, 18, 22, 255);
    const uint32 top = RGBAu32(26, 26, 32, 255);

    const float topH = U(76.0f);
    ui->Rect(0, 0, vw, vh, bg);
    ui->Rect(0, 0, vw, topH, top);
    ui->Rect(0, topH, vw, U(2.0f), RGBAu32(60, 60, 72, 180));

    ui->TextCentered(0, U(12.0f), vw, U(44.0f),
        "LEVEL SELECT", RGBAu32(245, 245, 250, 245), 1.15f * uiScale);

    // --------------------------------------------------------
    // Botones arriba (Back / Settings)
    // --------------------------------------------------------
    {
        const float bx = U(16.0f);
        const float by = U(22.0f);
        const float bw = U(94.0f);
        const float bh = U(32.0f);

        const uint32 c = RGBAu32(110, 70, 70, 220);
        if (ui->Button(bx, by, bw, bh, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f)))
            mgr->Request(SCENE_MAINMENU);

        ui->TextCentered(bx, by, bw, bh, "BACK", RGBAu32(250, 250, 250, 240), 0.85f * uiScale);
    }

    {
        const float bw = U(94.0f);
        const float bh = U(32.0f);
        const float bx = vw - U(16.0f) - bw;
        const float by = U(22.0f);

        const uint32 c = RGBAu32(70, 90, 140, 220);
        if (ui->Button(bx, by, bw, bh, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f)))
            mgr->OpenSettings(SCENE_LEVELSELECTOR);

        ui->TextCentered(bx, by, bw, bh, "SETTINGS", RGBAu32(250, 250, 250, 240), 0.85f * uiScale);
    }

    // --------------------------------------------------------
    // Total estrellas (pill centrada)
    // --------------------------------------------------------
    {
        const float pillW = U(70.0f);
        const float pillH = U(30.0f);
        const float pillX = vw * 0.5f - pillW * 0.5f;
        const float pillY = topH + U(14.0f);

        ui->Rect(pillX, pillY, pillW, pillH, RGBAu32(30, 30, 38, 240));
        ui->RectBorders(pillX, pillY, pillW, pillH, U(2.0f), RGBAu32(90, 90, 110, 90));

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d", app->progress.TotalStars());

        ui->Star(pillX + U(24.0f), pillY + pillH * 0.5f, U(9.0f), RGBAu32(250, 210, 70, 240));
        ui->Text(pillX + U(40.0f), pillY + U(2.0f), buf, RGBAu32(240, 240, 240, 240), 1.0f * uiScale);
    }

    // --------------------------------------------------------
    // Texturas (candado + thumbs)
    // --------------------------------------------------------
    static Texture2D lockTex;
    static bool lockLoaded = false;
    if (!lockLoaded) {
        lockLoaded = lockTex.Load("assets/sprites/ui/lock.png");
    }

    constexpr int LEVEL_COUNT = 15;

    // thumbs: assets/sprites/level_thumbs/level_01.png ... level_15.png
    static Texture2D thumbs[LEVEL_COUNT];
    static bool thumbTried[LEVEL_COUNT] = { false };

    // Grid 5x3
    constexpr int cols = 5;
    constexpr int rows = 3;

    // Espaciados
    const float padX = U(52.0f);
    const float padTop = (topH + U(14.0f) + U(30.0f) + U(18.0f)); // topH + pillY + pillH + margin
    const float padBottom = U(40.0f);
    const float gap = U(16.0f);

    const float gridW = vw - padX * 2.0f;
    const float gridH = vh - padTop - padBottom;

    const float tileW = (gridW - gap * (cols - 1)) / cols;
    const float tileH = (gridH - gap * (rows - 1)) / rows;

    // Scene targets
    const SceneId levelScenes[LEVEL_COUNT] = {
        SCENE_LEVEL1, SCENE_LEVEL2, SCENE_LEVEL3, SCENE_LEVEL4, SCENE_LEVEL5,
        SCENE_LEVEL6, SCENE_LEVEL7, SCENE_LEVEL8, SCENE_LEVEL9, SCENE_LEVEL10,
        SCENE_LEVEL11, SCENE_LEVEL12, SCENE_LEVEL13, SCENE_LEVEL14, SCENE_LEVEL15
    };

    for (int i = 0; i < LEVEL_COUNT; ++i) {
        const int rr = i / cols;
        const int cc = i % cols;

        const float x = padX + cc * (tileW + gap);
        const float y = padTop + rr * (tileH + gap);

        const bool unlocked = app->progress.IsLevelUnlocked(i);

        const uint32 base  = unlocked ? RGBAu32(44, 46, 56, 240) : RGBAu32(32, 33, 40, 240);
        const uint32 hover = unlocked ? MulRGBA(base, 1.12f) : base;
        const uint32 act   = unlocked ? MulRGBA(base, 0.88f) : base;

        // Sombra + botón
        ui->Rect(x + U(3.0f), y + U(5.0f), tileW, tileH, RGBAu32(0, 0, 0, 90));

        const bool clicked = ui->Button(x, y, tileW, tileH, base, hover, act);
        ui->RectBorders(x, y, tileW, tileH, U(2.0f), RGBAu32(90, 95, 110, unlocked ? 110 : 70));

        if (clicked && unlocked)
            mgr->Request(levelScenes[i]);

        // Thumb lazy load
        if (!thumbTried[i]) {
            thumbTried[i] = true;
            char p[128];
            std::snprintf(p, sizeof(p), "assets/sprites/level_thumbs/level_%02d.png", i + 1);
            thumbs[i].Load(p);
        }

        // ----------------------------------------------------
        // Thumbnail frame (16:9)
        // ----------------------------------------------------
        const float pad = U(10.0f);

        float frameX = x + pad;
        float frameY = y + pad;
        float frameW = tileW - pad * 2.0f;
        float frameH = frameW * (9.0f / 16.0f);

        const float reservedBelow = U(56.0f); // name (+ estrellas si unlocked)
        const float maxFrameH = tileH - pad - reservedBelow;

        if (frameH > maxFrameH) frameH = maxFrameH;
        if (frameH < U(8.0f)) frameH = U(8.0f);

        ui->Rect(frameX, frameY, frameW, frameH, RGBAu32(14, 14, 18, 200));
        ui->RectBorders(frameX, frameY, frameW, frameH, U(2.0f), RGBAu32(110, 115, 135, 70));

        const float inner = U(4.0f);
        const float tx = frameX + inner;
        const float ty = frameY + inner;
        const float tw = frameW - inner * 2.0f;
        const float th = frameH - inner * 2.0f;

        // Thumbnail (si no hay imagen, rect)
        if (thumbs[i].id != 0) {
            const uint32 tint = unlocked ? 0xFFFFFFFF : RGBAu32(200, 200, 200, 130);
            ui->Image(thumbs[i], tx, ty, tw, th, tint);
        } else {
            const uint32 c = unlocked ? RGBAu32(255, 255, 255, 180) : RGBAu32(55, 55, 62, 180);
            ui->Rect(tx, ty, tw, th, c);
        }

        // ----------------------------------------------------
        // Texto del nivel
        // ----------------------------------------------------
        char name[32];
        std::snprintf(name, sizeof(name), "LEVEL %02d", i + 1);

        const float nameY = frameY + frameH + U(5.0f);
        ui->Text(x + U(12.0f), nameY, name,
            unlocked ? RGBAu32(240, 240, 245, 235) : RGBAu32(185, 185, 195, 170),
            unlocked ? 0.90f * uiScale : 0.60f * uiScale);

        // ----------------------------------------------------
        // Estrellas (solo si está desbloqueado)
        // ----------------------------------------------------
        if (unlocked) {
            const uint8 earned = app->progress.StarsFor(i);

            const float starY = y + tileH - U(18.0f);
            const float starR = U(8.0f);
            const float stepX = U(22.0f);

            // alineadas a la derecha
            const float startX = x + tileW - U(12.0f) - stepX * 2.0f;

            for (int st = 0; st < 3; ++st) {
                const float cx = startX + st * stepX;

                if (st < (int)earned) ui->Star(cx, starY, starR, RGBAu32(250, 210, 70, 240));
                else ui->StarOutline(cx, starY, starR, RGBAu32(200, 200, 210, 210), base);
            }
        }

        // ----------------------------------------------------
        // Locked overlay + candado + requisito
        // ----------------------------------------------------
        if (!unlocked) {
            ui->Rect(x, y, tileW, tileH, RGBAu32(0, 0, 0, 120));

            // Candado centrado en el thumbnail
            if (lockLoaded && lockTex.id != 0) {
                const float lockS = std::fmin(frameW, frameH) * 0.8f;
                const float lx = frameX + (frameW - lockS) * 0.5f;
                const float ly = frameY + (frameH - lockS) * 0.5f;
                ui->Image(lockTex, lx, ly, lockS, lockS, RGBAu32(255, 255, 255, 210));
            }

            // Pill con el requisito
            char req[32];
            std::snprintf(req, sizeof(req), "NEED %d STARS", app->progress.UnlockRequirement(i));

            const float rw = tileW * 0.78f;
            const float rh = U(22.0f);
            const float rx = x + (tileW - rw) * 0.5f;
            const float ry = y + tileH - rh - U(10.0f);

            ui->Rect(rx, ry, rw, rh, RGBAu32(20, 20, 26, 235));
            ui->RectBorders(rx, ry, rw, rh, U(2.0f), RGBAu32(120, 120, 140, 80));
            ui->TextCentered(rx, ry, rw, rh, req, RGBAu32(240, 240, 245, 225), 0.72f * uiScale);
        }
    }
}
