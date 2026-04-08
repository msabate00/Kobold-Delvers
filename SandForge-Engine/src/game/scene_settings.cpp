#include "scene_settings.h"
#include "scene_manager.h"
#include "app/app.h"
#include "app/settings.h"
#include "core/input.h"
#include "ui/ui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

static void PercentText(float v, char* out, int cap)
{
    int p = (int)(v * 100.0f + 0.5f);
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    std::snprintf(out, cap, "%d%%", p);
}

void Scene_Settings::OnEnter()
{
}

void Scene_Settings::Update(float)
{
    if (app->input->KeyDown(GLFW_KEY_ESCAPE)) {
        if (app->settings) app->settings->Save();
        mgr->Request(mgr->SettingsReturnTo());
    }
}

void Scene_Settings::DrawUI(int&, Material&)
{
    UI* ui = app->ui;
    Settings* st = app->settings;
    SettingsData* s = st ? &st->data : nullptr;

    const float vw = (float)app->framebufferSize.x;
    const float vh = (float)app->framebufferSize.y;

    ui->Cursor();

    ui->Rect(0, 0, vw, vh, RGBAu32(20, 20, 26, 255));

    float scale = std::fmin(vw / 1280.0f, vh / 720.0f);
    if (scale < 0.85f) scale = 0.85f;
    if (scale > 1.15f) scale = 1.15f;
    auto S = [&](float v) { return v * scale; };

    // Caja
    const float panelW = std::fmin(S(740.0f), vw * 0.92f);
    const float panelH = std::fmin(S(620.0f), vh * 0.92f);
    const float px = std::floor((vw - panelW) * 0.5f);
    const float py = std::floor((vh - panelH) * 0.5f);

    ui->Rect(px, py, panelW, panelH, RGBAu32(30, 30, 36, 235));
    ui->RectBorders(px, py, panelW, panelH, 2.0f, RGBAu32(70, 70, 80, 200));

    //Titulo
    {
        const float w = std::fmin(S(300.0f), panelW * 0.50f);
        const float h = S(46.0f);
        const float x = px + (panelW - w) * 0.5f;
        const float y = py + S(6.0f);

        ui->Image(ui->interfaceTex, x, y, w, h,
            ui->buttonUp, RGBAu32(244, 224, 190, 255), 5);

        ui->TextCentered(x + S(2.0f), y - S(2.0f), w, h,
            "SETTINGS", RGBAu32(248, 245, 240, 245), 1.02f * scale);
    }

    float x = px + S(40.0f);
    float y = py + S(82.0f);

    const float labelW = S(160.0f);
    const float sliderW = std::fmax(S(120.0f), panelW - (S(40.0f) * 2.0f + labelW + S(96.0f)));
    const float rowH = S(54.0f);

    const float sliderH = S(22.0f);
    const float valueW = S(74.0f);
    const float valueH = S(34.0f);

    const float smallBtnW = S(112.0f);
    const float smallBtnH = S(34.0f);

    const float modeBtnW = S(148.0f);
    const float modeBtnH = S(34.0f);
    const float modeGap = S(10.0f);

    //Music volume
    {
        ui->Text(x, y + S(5.0f), "Music Volume", RGBAu32(236, 232, 224, 235), 0.90f * scale);

        float before = s ? s->musicVolume : 1.0f;
        float v = before;

        ui->Slider(x + labelW, y + S(1.0f), sliderW, sliderH,
            0.0f, 1.0f, v, true, RGBAu32(255, 244, 225, 255));

        if (s) {
            s->musicVolume = v;
            if (s->musicVolume != before) st->ApplyAudio();
        }

        char buf[16];
        PercentText(v, buf, (int)sizeof(buf));

        const float pillX = x + labelW + sliderW + S(14.0f);
        const float pillY = y - S(4.0f);

        ui->TextCentered(pillX + S(1.0f), pillY - S(2.0f), valueW, valueH,
            buf, RGBAu32(248, 245, 240, 245), 0.74f * scale);

        y += rowH;
    }

    //SFX volume
    {
        ui->Text(x, y + S(5.0f), "SFX Volume", RGBAu32(236, 232, 224, 235), 0.90f * scale);

        float before = s ? s->sfxVolume : 1.0f;
        float v = before;

        ui->Slider(x + labelW, y + S(1.0f), sliderW, sliderH,
            0.0f, 1.0f, v, true, RGBAu32(255, 244, 225, 255));

        if (s) {
            s->sfxVolume = v;
            if (s->sfxVolume != before) st->ApplyAudio();
        }

        char buf[16];
        PercentText(v, buf, (int)sizeof(buf));

        const float pillX = x + labelW + sliderW + S(14.0f);
        const float pillY = y - S(4.0f);

        ui->TextCentered(pillX + S(1.0f), pillY - S(2.0f), valueW, valueH,
            buf, RGBAu32(248, 245, 240, 245), 0.74f * scale);

        y += rowH;
    }

    //VSync
    {
        ui->Text(x, y + S(5.0f), "VSync", RGBAu32(236, 232, 224, 235), 0.90f * scale);

        const float bx = x + labelW;
        const float by = y - S(4.0f);

        const bool on = s ? s->vsync : true;
        const uint32 tint = on ? RGBAu32(186, 220, 180, 255) : RGBAu32(232, 205, 195, 255);

        if (ui->ImageButton(ui->interfaceTex, bx, by, smallBtnW, smallBtnH,
            ui->buttonUp, ui->buttonDown, ui->buttonDown, tint, 6) && s) {
            s->vsync = !s->vsync;
            st->ApplyGraphics();
        }

        ui->TextCentered(bx + S(2.0f), by - S(2.0f), smallBtnW, smallBtnH,
            on ? "ON" : "OFF", RGBAu32(248, 245, 240, 245), 0.78f * scale);

        y += rowH;
    }

    //Window mode
    {
        ui->Text(x, y + S(5.0f), "Window Mode", RGBAu32(236, 232, 224, 235), 0.90f * scale);

        const float by = y - S(4.0f);
        float bx = x + labelW;

        {
            bool selected = s && s->windowMode == WindowMode::Windowed;
            uint32 tint = selected ? RGBAu32(236, 218, 190, 255) : RGBAu32(185, 210, 245, 255);

            if (ui->ImageButton(ui->interfaceTex, bx, by, modeBtnW, modeBtnH,
                ui->buttonUp, ui->buttonDown, ui->buttonDown, tint, 6) && s) {
                if (s->windowMode != WindowMode::Windowed) {
                    s->windowMode = WindowMode::Windowed;
                    st->ApplyGraphics();
                }
            }

            ui->TextCentered(bx + S(2.0f), by - S(2.0f), modeBtnW, modeBtnH,
                "WINDOWED", RGBAu32(248, 245, 240, 245), 0.72f * scale);

            bx += modeBtnW + modeGap;
        }

        {
            bool selected = s && s->windowMode == WindowMode::Fullscreen;
            uint32 tint = selected ? RGBAu32(236, 218, 190, 255) : RGBAu32(185, 210, 245, 255);

            if (ui->ImageButton(ui->interfaceTex, bx, by, modeBtnW, modeBtnH,
                ui->buttonUp, ui->buttonDown, ui->buttonDown, tint, 6) && s) {
                if (s->windowMode != WindowMode::Fullscreen) {
                    s->windowMode = WindowMode::Fullscreen;
                    st->ApplyGraphics();
                }
            }

            ui->TextCentered(bx + S(2.0f), by - S(2.0f), modeBtnW, modeBtnH,
                "FULLSCREEN", RGBAu32(248, 245, 240, 245), 0.72f * scale);

            bx += modeBtnW + modeGap;
        }

        {
            bool selected = s && s->windowMode == WindowMode::Borderless;
            uint32 tint = selected ? RGBAu32(236, 218, 190, 255) : RGBAu32(185, 210, 245, 255);

            if (ui->ImageButton(ui->interfaceTex, bx, by, modeBtnW, modeBtnH,
                ui->buttonUp, ui->buttonDown, ui->buttonDown, tint, 6) && s) {
                if (s->windowMode != WindowMode::Borderless) {
                    s->windowMode = WindowMode::Borderless;
                    st->ApplyGraphics();
                }
            }

            ui->TextCentered(bx + S(2.0f), by - S(2.0f), modeBtnW, modeBtnH,
                "BORDERLESS", RGBAu32(248, 245, 240, 245), 0.72f * scale);
        }

        y += rowH;
    }

    //Extra
    {
        ui->Text(x, y + S(5.0f), "Show Chunks", RGBAu32(236, 232, 224, 235), 0.90f * scale);

        const float bx = x + labelW;
        const float by = y - S(4.0f);

        const bool on = (app->showChunks != 0);
        const uint32 tint = on ? RGBAu32(186, 220, 180, 255) : RGBAu32(236, 218, 190, 255);

        if (ui->ImageButton(ui->interfaceTex, bx, by, smallBtnW, smallBtnH,
            ui->buttonUp, ui->buttonDown, ui->buttonDown, tint, 6)) {
            app->showChunks = on ? 0 : 1;
        }

        ui->TextCentered(bx + S(2.0f), by - S(2.0f), smallBtnW, smallBtnH,
            on ? "ON" : "OFF", RGBAu32(248, 245, 240, 245), 0.78f * scale);

        y += rowH;
    }

    {
        ui->Text(x, y + S(5.0f), "Show Hitbox", RGBAu32(236, 232, 224, 235), 0.90f * scale);

        const float bx = x + labelW;
        const float by = y - S(4.0f);

        const bool on = (app->showHitbox != 0);
        const uint32 tint = on ? RGBAu32(186, 220, 180, 255) : RGBAu32(236, 218, 190, 255);

        if (ui->ImageButton(ui->interfaceTex, bx, by, smallBtnW, smallBtnH,
            ui->buttonUp, ui->buttonDown, ui->buttonDown, tint, 6)) {
            app->showHitbox = on ? 0 : 1;
        }

        ui->TextCentered(bx + S(2.0f), by - S(2.0f), smallBtnW, smallBtnH,
            on ? "ON" : "OFF", RGBAu32(248, 245, 240, 245), 0.78f * scale);

        y += rowH;
    }

    // Bottom buttons
    {
        const float bw = S(148.0f);
        const float bh = S(38.0f);
        const float by = py + panelH - bh - S(18.0f);

        // RESET
        {
            const float bx = px + S(18.0f);
            if (ui->ImageButton(ui->interfaceTex, bx, by, bw, bh,
                ui->buttonUp, ui->buttonDown, ui->buttonDown,
                RGBAu32(236, 218, 190, 255), 6) && st) {
                st->ResetToDefaults();
                st->ApplyGraphics();
                st->ApplyAudio();
            }

            ui->TextCentered(bx + S(2.0f), by - S(2.0f), bw, bh,
                "RESET", RGBAu32(248, 245, 240, 245), 0.80f * scale);
        }

        // BACK
        {
            const float bx = px + panelW - bw - S(18.0f);
            if (ui->ImageButton(ui->interfaceTex, bx, by, bw, bh,
                ui->buttonUp, ui->buttonDown, ui->buttonDown,
                RGBAu32(236, 198, 188, 255), 6)) {
                if (st) st->Save();
                mgr->Request(mgr->SettingsReturnTo());
            }

            ui->TextCentered(bx + S(2.0f), by - S(2.0f), bw, bh,
                "BACK", RGBAu32(248, 245, 240, 245), 0.80f * scale);
        }

        ui->Text(px + S(18.0f), by - S(22.0f),
            "ESC: Back", RGBAu32(226, 220, 210, 180), 0.74f * scale);
    }
}