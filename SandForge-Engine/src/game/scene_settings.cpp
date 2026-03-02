#include "scene_settings.h"
#include "scene_manager.h"
#include "app/app.h"
#include "app/settings.h"
#include "core/input.h"
#include "ui/ui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>


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
    float vw = (float)app->framebufferSize.x;
    float vh = (float)app->framebufferSize.y;

    ui->Rect(0, 0, vw, vh, RGBAu32(18, 18, 22, 255));

    const float panelW = std::fmin(740.0f, vw * 0.92f);
    const float panelH = std::fmin(620.0f, vh * 0.92f);
    const float px = (vw - panelW) * 0.5f;
    const float py = (vh - panelH) * 0.5f;

    ui->Rect(px, py, panelW, panelH, RGBAu32(30, 30, 36, 235));
    ui->RectBorders(px, py, panelW, panelH, 2.0f, RGBAu32(70, 70, 80, 200));

    ui->TextCentered(px, py + 10.0f, panelW, 44.0f, "SETTINGS", RGBAu32(245, 245, 245, 240), 1.2f);

    float x = px + 40.0f;
    float y = py + 78.0f;

    const float labelW = 160.0f;
    const float sliderW = std::fmax(120.0f, panelW - (40.0f * 2.0f + labelW + 90.0f));
    const float rowH = 54.0f;

    auto pctText = [](float v, char* out, int cap) {
        int p = (int)(v * 100.0f + 0.5f);
        if (p < 0) p = 0;
        if (p > 100) p = 100;
        std::snprintf(out, cap, "%d%%", p);
    };

    Settings* st = app->settings;
    SettingsData* s = st ? &st->data : nullptr;

    //Music volume
    {
        ui->Text(x, y + 6.0f, "Music Volume", RGBAu32(235, 235, 235, 230), 0.9f);
        float before = s ? s->musicVolume : 1.0f;
        float v = before;
        ui->Slider(x + labelW, y, sliderW, 24.0f, 0.0f, 1.0f, v,
            RGBAu32(70, 70, 80, 220), RGBAu32(210, 210, 220, 240));
        if (s) {
            s->musicVolume = v;
            if (s->musicVolume != before) st->ApplyAudio();
        }

        char buf[16]; pctText(v, buf, (int)sizeof(buf));
        ui->Text(x + labelW + sliderW + 22.0f, y + 3.0f, buf, RGBAu32(235, 235, 235, 210), 0.9f);
        y += rowH;
    }

    //SFX volume
    {
        ui->Text(x, y + 6.0f, "SFX Volume", RGBAu32(235, 235, 235, 230), 0.9f);
        float before = s ? s->sfxVolume : 1.0f;
        float v = before;
        ui->Slider(x + labelW, y, sliderW, 24.0f, 0.0f, 1.0f, v,
            RGBAu32(70, 70, 80, 220), RGBAu32(210, 210, 220, 240));
        if (s) {
            s->sfxVolume = v;
            if (s->sfxVolume != before) st->ApplyAudio();
        }

        char buf[16]; pctText(v, buf, (int)sizeof(buf));
        ui->Text(x + labelW + sliderW + 22.0f, y + 3.0f, buf, RGBAu32(235, 235, 235, 210), 0.9f);
        y += rowH;
    }

    //VSync
    {
        ui->Text(x, y + 6.0f, "VSync", RGBAu32(235, 235, 235, 230), 0.9f);

        const float bw = 120.0f;
        const float bh = 28.0f;
        const float bx = x + labelW;
        const float by = y;

        const bool on = s ? s->vsync : true;
        const uint32 c = on ? RGBAu32(80, 120, 80, 230) : RGBAu32(120, 70, 70, 230);
        const uint32 ch = MulRGBA(c, 1.15f);
        const uint32 ca = MulRGBA(c, 0.85f);

        if (ui->Button(bx, by, bw, bh, c, ch, ca) && s) {
            s->vsync = !s->vsync;
            st->ApplyGraphics();
        }
        ui->TextCentered(bx, by, bw, bh, on ? "ON" : "OFF", RGBAu32(245, 245, 245, 240), 0.85f);
        y += rowH;
    }

    //Window mode
    {
        ui->Text(x, y + 6.0f, "Window Mode", RGBAu32(235, 235, 235, 230), 0.9f);

        const float bw = 150.0f;
        const float bh = 28.0f;
        float bx = x + labelW;

        auto modeBtn = [&](WindowMode m, const char* label) {
            const bool sel = (s && s->windowMode == m);
            uint32 c = sel ? RGBAu32(70, 90, 140, 235) : RGBAu32(60, 60, 70, 220);
            uint32 ch = MulRGBA(c, 1.15f);
            uint32 ca = MulRGBA(c, 0.85f);
            if (ui->Button(bx, y, bw, bh, c, ch, ca) && s) {
                s->windowMode = m;
                st->ApplyGraphics();
            }
            ui->TextCentered(bx, y, bw, bh, label, RGBAu32(245, 245, 245, 240), 0.78f);
            bx += bw + 10.0f;
        };

        modeBtn(WindowMode::Windowed, "WINDOWED");
        modeBtn(WindowMode::Fullscreen, "FULLSCREEN");
        modeBtn(WindowMode::Borderless, "BORDERLESS");

        y += rowH;
    }

    //Extra
    {
        ui->Text(x, y + 6.0f, "Show Chunks", RGBAu32(235, 235, 235, 230), 0.9f);
        const float bw = 120.0f, bh = 28.0f;
        const float bx = x + labelW, by = y;
        const bool on = (app->showChunks != 0);
        uint32 c = on ? RGBAu32(80, 120, 80, 230) : RGBAu32(60, 60, 70, 220);
        if (ui->Button(bx, by, bw, bh, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f))) app->showChunks = on ? 0 : 1;
        ui->TextCentered(bx, by, bw, bh, on ? "ON" : "OFF", RGBAu32(245, 245, 245, 240), 0.85f);
        y += rowH;

        ui->Text(x, y + 6.0f, "Show Hitbox", RGBAu32(235, 235, 235, 230), 0.9f);
        const float bx2 = x + labelW, by2 = y;
        const bool on2 = (app->showHitbox != 0);
        uint32 c2 = on2 ? RGBAu32(80, 120, 80, 230) : RGBAu32(60, 60, 70, 220);
        if (ui->Button(bx2, by2, bw, bh, c2, MulRGBA(c2, 1.15f), MulRGBA(c2, 0.85f))) app->showHitbox = on2 ? 0 : 1;
        ui->TextCentered(bx2, by2, bw, bh, on2 ? "ON" : "OFF", RGBAu32(245, 245, 245, 240), 0.85f);
        y += rowH;
    }

    // Bottom buttons
    {
        const float bh = 40.0f;
        const float bw = 140.0f;
        const float by = py + panelH - bh - 18.0f;

        // RESET
        {
            const float bx = px + 18.0f;
            const uint32 c = RGBAu32(110, 110, 120, 220);
            if (ui->Button(bx, by, bw, bh, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f)) && st) {
                st->ResetToDefaults();
                st->ApplyGraphics();
                st->ApplyAudio();
            }
            ui->TextCentered(bx, by, bw, bh, "RESET", RGBAu32(245, 245, 245, 240), 0.9f);
        }

        // BACK
        {
            const float bx = px + panelW - bw - 18.0f;
            const uint32 c = RGBAu32(120, 70, 70, 220);
            if (ui->Button(bx, by, bw, bh, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f))) {
                if (st) st->Save();
                mgr->Request(mgr->SettingsReturnTo());
            }
            ui->TextCentered(bx, by, bw, bh, "BACK", RGBAu32(245, 245, 245, 240), 0.9f);
        }

        ui->Text(px + 18.0f, by - 24.0f, "ESC: Back", RGBAu32(220, 220, 220, 170), 0.8f);
    }
}
