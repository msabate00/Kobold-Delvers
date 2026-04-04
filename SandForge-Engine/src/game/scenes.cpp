#include "scenes.h"

#include "scene_manager.h"

#include "app/app.h"
#include "core/engine.h"
#include "core/input.h"
#include "ui/ui.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

static uint32_t Hash32(const std::string& s)
{
    uint32_t h = 2166136261u;
    for (char c : s) {
        h ^= (uint8_t)c;
        h *= 16777619u;
    }
    return h;
}

static uint32_t ColorForName(const std::string& s)
{
    uint32_t h = Hash32(s);
    unsigned r = 60 + (h & 0x7Fu);
    unsigned g = 60 + ((h >> 8) & 0x7Fu);
    unsigned b = 60 + ((h >> 16) & 0x7Fu);
    return RGBAu32(r, g, b, 220);
}

void Scene_MainMenu::DrawUI(int&, Material&)
{
    UI* ui = app->ui;
    float vw = (float)app->framebufferSize.x;
    float vh = (float)app->framebufferSize.y;

    //bACKGROUND
    ui->Rect(0, 0, vw, vh, RGBAu32(50, 47, 44, 255));

    ui->Cursor();

    // Logo
    static Texture2D sLogo;
    static bool sLogoLoaded = false;
    if (!sLogoLoaded) {
        sLogoLoaded = sLogo.Load("assets/sprites/logo.png");
    }

    float logoW = 768.0f;
    float logoH = 498.0f;

    float maxW = vw * 0.40f;
    if (logoW > maxW) {
        float sc = maxW / logoW;
        logoW *= sc;
        logoH *= sc;
    }

    float logoX = (vw - logoW) * 0.5f;
    float logoY = 40.0f;

    if (sLogoLoaded) {
        ui->Image(sLogo, logoX, logoY, logoW, logoH, 0xFFFFFFFF);
    }

    //Botones
    float bw = 240.0f;
    float bh = 48.0f;
    float spacing = 16.0f;
    float cx = (vw - bw) * 0.5f;
    float cy = logoY + logoH + 28.0f;

    uint32 base = RGBAu32(80, 80, 90, 220);
    uint32 hover = MulRGBA(base, 1.15f);
    uint32 act = MulRGBA(base, 0.85f);

    if (ui->Button(cx, cy, bw, bh, base, hover, act)) mgr->Request(SCENE_LEVELSELECTOR);
    ui->TextCentered(cx, cy, bw, bh, "LEVELS", RGBAu32(240, 240, 240, 230), 1.0f);

    cy += bh + spacing;
    if (ui->Button(cx, cy, bw, bh, RGBAu32(70, 90, 70, 220), RGBAu32(90, 120, 90, 230), RGBAu32(60, 80, 60, 220))) mgr->Request(SCENE_SANDBOX);
    ui->TextCentered(cx, cy, bw, bh, "SANDBOX", RGBAu32(240, 240, 240, 230), 1.0f);

    cy += bh + spacing;
    uint32 cSet = RGBAu32(70, 90, 140, 220);
    if (ui->Button(cx, cy, bw, bh, cSet, MulRGBA(cSet, 1.15f), MulRGBA(cSet, 0.85f))) mgr->OpenSettings(SCENE_MAINMENU);
    ui->TextCentered(cx, cy, bw, bh, "SETTINGS", RGBAu32(240, 240, 240, 230), 1.0f);

    cy += bh + spacing;
    if (ui->Button(cx, cy, bw, bh, RGBAu32(120, 70, 70, 220), RGBAu32(150, 90, 90, 230), RGBAu32(100, 60, 60, 220))) app->RequestQuit();
    ui->TextCentered(cx, cy, bw, bh, "QUIT", RGBAu32(240, 240, 240, 230), 1.0f);
}

int Scene_Level::LevelIndex() const
{
    return (int)id - (int)SCENE_LEVEL1;
}

void Scene_Level::OnEnter()
{
    app->engine->paused = false;
    app->engine->stepOnce = false;
    app->engine->levelCellsProtection = true;

    levelFinished = false;
    levelResultSaved = false;
    bonusStarEarned = false;
    budgetStarEarned = false;
    levelStarsEarned = 0;

    // Load assigned level
    if (!levelPath.empty()) {
        app->engine->LoadLevel(levelPath.c_str());
    }
    else {
        app->engine->ResetLevelSession();
    }
    app->ResetCamera();
}

void Scene_Level::OnExit()
{
    app->engine->StopPaint();
    app->engine->paused = false;
}

void Scene_Level::CheckLevelCompleted()
{
    if (levelFinished) return;
    if (app->engine->CapturedGoalCount() < 5) return;

    levelFinished = true;
    app->engine->paused = true;

    bonusStarEarned = app->engine->BonusTriggered();
    budgetStarEarned = app->engine->MaterialBudgetEnabled() &&
        app->engine->MaterialUsed() <= app->engine->LevelMaterialBudgetStar();

    levelStarsEarned = (uint8)(1 + (bonusStarEarned ? 1 : 0) + (budgetStarEarned ? 1 : 0));

    if (levelResultSaved) return;

    const int levelIndex = LevelIndex();
    if (levelIndex >= 0 && app->progress.StarsFor(levelIndex) < levelStarsEarned) {
        if (app->progress.SetStarsFor(levelIndex, levelStarsEarned)) {
            app->progress.Save();
        }
    }
    levelResultSaved = true;
}

void Scene_Level::Update(float)
{
    CheckLevelCompleted();

    if (levelFinished) {
        if (app->input->KeyDown(GLFW_KEY_R)) {
            OnEnter();
            return;
        }
        if (app->input->KeyDown(GLFW_KEY_ESCAPE) || app->input->KeyDown(GLFW_KEY_ENTER)) {
            mgr->Request(SCENE_LEVELSELECTOR);
        }
        return;
    }

    if (app->input->KeyDown(GLFW_KEY_ESCAPE)) mgr->Request(SCENE_LEVELSELECTOR);
}

void Scene_Level::DrawMaterialBudgetBar()
{
    if (!app->engine->MaterialBudgetEnabled()) return;

    UI* ui = app->ui;

    const float maxBudget = (float)std::fmax(1, app->engine->LevelMaterialBudgetMax());
    const float used = (float)std::fmax(0, app->engine->MaterialUsed());
    const float remaining = std::fmax(0.0f, maxBudget - used);
    const float fill = std::clamp(remaining / maxBudget, 0.0f, 1.0f);

    const float starLimit = (float)std::clamp(app->engine->LevelMaterialBudgetStar(), 0, app->engine->LevelMaterialBudgetMax());
    const float starFill = std::clamp((maxBudget - starLimit) / maxBudget, 0.0f, 1.0f);

    const float barX = 12.0f;
    const float barY = 52.0f;
    const float barW = 36.0f;
    const float barH = 300.0f;

    ui->Rect(barX - 4.0f, barY - 4.0f, barW + 8.0f, barH + 8.0f, RGBAu32(18, 18, 18, 215));
    ui->Rect(barX, barY, barW, barH, RGBAu32(50, 50, 55, 240));

    const float fillH = barH * fill;
    if (fillH > 0.0f) {
        ui->Rect(barX, barY + (barH - fillH), barW, fillH,
            budgetStarEarned || (!levelFinished && used <= starLimit) ? RGBAu32(235, 210, 90, 245) : RGBAu32(180, 75, 75, 245));
    }

    const float markY = barY + barH * starFill;
    ui->Rect(barX - 4.0f, markY - 1.0f, barW + 8.0f, 2.0f, RGBAu32(245, 245, 245, 230));
    ui->RectBorders(barX, barY, barW, barH, 2.0f, RGBAu32(220, 220, 220, 120));

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d/%d", app->engine->MaterialUsed(), app->engine->LevelMaterialBudgetMax());
    ui->Text(barX + 28.0f, barY + barH - 16.0f, buf, RGBAu32(245, 245, 245, 235), 0.70f);
    ui->Text(barX - 2.0f, barY - 18.0f, "MAT", RGBAu32(245, 245, 245, 220), 0.70f);
    std::snprintf(buf, sizeof(buf), "%d", app->engine->LevelMaterialBudgetStar());
    ui->Text(barX + 28.0f, markY - 8.0f, buf, RGBAu32(235, 235, 235, 220), 0.65f);
}

void Scene_Level::DrawLevelCompleteModal()
{
    if (!levelFinished) return;

    UI* ui = app->ui;
    const float vw = (float)app->framebufferSize.x;
    const float vh = (float)app->framebufferSize.y;

    ui->Button(0.0f, 0.0f, vw, vh, RGBAu32(0, 0, 0, 0), RGBAu32(0, 0, 0, 0), RGBAu32(0, 0, 0, 0));
    ui->Rect(0.0f, 0.0f, vw, vh, RGBAu32(0, 0, 0, 150));

    const float panelW = 360.0f;
    const float panelH = 220.0f;
    const float px = (vw - panelW) * 0.5f;
    const float py = (vh - panelH) * 0.5f - 40.0f;

    ui->Rect(px, py, panelW, panelH, RGBAu32(34, 34, 38, 245));
    ui->RectBorders(px, py, panelW, panelH, 3.0f, RGBAu32(235, 235, 235, 60));
    ui->TextCentered(px, py + 14.0f, panelW, 34.0f, "LEVEL COMPLETE", RGBAu32(250, 250, 250, 245), 1.0f);

    const float starY = py + 78.0f;
    const float starR = 22.0f;
    const float starGap = 70.0f;
    const float startX = px + panelW * 0.5f - starGap;

    for (int i = 0; i < 3; ++i) {
        const float sx = startX + i * starGap;
        const bool filled = i < (int)levelStarsEarned;
        ui->StarOutline(sx, starY, starR,
            filled ? RGBAu32(250, 210, 70, 245) : RGBAu32(170, 170, 175, 200),
            filled ? RGBAu32(250, 210, 70, 245) : RGBAu32(70, 70, 78, 220));
    }

    char line[128];
    std::snprintf(line, sizeof(line), "Bonus: %s", bonusStarEarned ? "OK" : "NO");
    ui->TextCentered(px, py + 118.0f, panelW, 18.0f, line, RGBAu32(235, 235, 235, 225), 0.75f);

    if (app->engine->MaterialBudgetEnabled()) {
        std::snprintf(line, sizeof(line), "Material: %d / %d", app->engine->MaterialUsed(), app->engine->LevelMaterialBudgetStar());
    }
    else {
        std::snprintf(line, sizeof(line), "Material: no limit set");
    }
    ui->TextCentered(px, py + 138.0f, panelW, 18.0f, line, RGBAu32(235, 235, 235, 225), 0.75f);

    const float btnW = 120.0f;
    const float btnH = 34.0f;
    const float btnY = py + panelH - 50.0f;
    const float retryX = px + 44.0f;
    const float backX = px + panelW - btnW - 44.0f;

    if (ui->Button(retryX, btnY, btnW, btnH, RGBAu32(90, 120, 90, 230), RGBAu32(110, 145, 110, 240), RGBAu32(70, 95, 70, 230))) {
        OnEnter();
    }
    ui->TextCentered(retryX, btnY, btnW, btnH, "RETRY", RGBAu32(245, 245, 245, 240), 0.85f);

    if (ui->Button(backX, btnY, btnW, btnH, RGBAu32(120, 80, 80, 230), RGBAu32(150, 105, 105, 240), RGBAu32(95, 60, 60, 230))) {
        mgr->Request(SCENE_LEVELSELECTOR);
    }
    ui->TextCentered(backX, btnY, btnW, btnH, "LEVELS", RGBAu32(245, 245, 245, 240), 0.85f);
}

void Scene_Level::DrawUI(int& brushSize, Material& brushMat)
{
    app->ui->Draw(brushSize, brushMat);
    DrawMaterialBudgetBar();



    app->ui->Image(app->ui->interfaceTex, 0, app->framebufferSize.y - 150, app->framebufferSize.x, 150, AtlasRectPx{0, 930, 1080, 150}, RGBAu32(255, 255, 255, 255), -10);


    //sLIDER
    float y = app->framebufferSize.y - 50.0f;
    float x = ((float)app->framebufferSize.x / 2) - (((36.0 + 16) * Material::COUNT) / 2);


    x += 12.0f; float bx = x, bw = 200.0f, bh = 20.0f; float v = (float)brushSize;
    app->ui->Slider(x, y, 200.0f, 20.0f, 1.0f, 64.0f, v, app->ui->interfaceTex, AtlasRectPx{ 374,877,21,42 }, AtlasRectPx{ 135,890,220,16 }, AtlasRectPx{ 413, 890, 220,16 });
    brushSize = (int)(v + 0.5f);



    x = ((float)app->framebufferSize.x / 2) - (((36.0+16)*Material::COUNT)/2);
    y = app->framebufferSize.y - 100.0f;
    float s = 28.0f;

    //Down Materials
    {
        app->ui->Rect(0, app->framebufferSize.y - 150, app->framebufferSize.x, 150, RGBAu32(24, 24, 24, 255));

        auto makeBtnColor = [&](uint32 base, int i = 0) {
            uint32 h = MulRGBA(base, 1.15f), a = MulRGBA(base, 0.85f);

            //Background
            if (app->engine->brushMat == i) {
                app->ui->Image(app->ui->interfaceTex, x-27.5, y-27.5, 55, 55, AtlasRectPx{ 5,871,55,55 }, RGBAu32(255,255,255,255), 4);
            }
            else {
                app->ui->Image(app->ui->interfaceTex, x-21, y-21, 42, 42, AtlasRectPx{ 66,877,42,42 }, RGBAu32(255, 255, 255, 255), 4);
            }


            bool clicked = app->ui->ImageButton(app->ui->matAtlas, x-16, y-16, 32, 32, AtlasRectPx{32 * i,0,32,32}, base, h, a, 5);
            x += 32 + 16.0f;
            /*if ((i+1) % 5 == 0) {
                y += 32 + 16;
                x = (float)app->framebufferSize.x * 0.20f - 36.0f;
            }*/
            
            return clicked; 
        };

        for (int i = 0; i < 256; ++i) {
            const MatProps& mp = matProps((uint8)i);

            if (mp.name.length() > 0) {
                //uint32 c = RGBAu32(mp.color.r, mp.color.g, mp.color.b, 230);
                uint32 c = RGBAu32(255, 255, 255, 255);
                if (makeBtnColor(c, i)) brushMat = (Material)i;
            }
        }

        x += 8.0f;

        if (app->engine->paused && !levelFinished) {
            if (makeBtnColor(RGBAu32(250, 200, 200, 230))) app->engine->paused = false;
            if (makeBtnColor(RGBAu32(180, 220, 180, 230))) app->engine->stepOnce = true;
        }
        else if (!levelFinished) {
            if (makeBtnColor(RGBAu32(200, 200, 200, 230))) app->engine->paused = true;
        }
    }

    //Back
    x = (float)app->framebufferSize.x - 36.0f;
    y = 8.0f;
    s = 28.0f;
    if (app->ui->Button(x, y, s, s, RGBAu32(200, 80, 80, 230), RGBAu32(240, 120, 120, 240), RGBAu32(170, 60, 60, 230))) {
        mgr->Request(SCENE_LEVELSELECTOR);
    }
    app->ui->TextCentered(x, y, s, s, "<", RGBAu32(250, 250, 250, 240), 0.85f);

    //Settings
    float sx = x - 34.0f;
    uint32 cSet = RGBAu32(70, 90, 140, 220);
    if (app->ui->Button(sx, y, s, s, cSet, MulRGBA(cSet, 1.15f), MulRGBA(cSet, 0.85f))) {
        mgr->OpenSettings(GetId());
    }
    app->ui->TextCentered(sx, y, s, s, "S", RGBAu32(250, 250, 250, 240), 0.85f);

    DrawLevelCompleteModal();
}

Scene_Sandbox::Scene_Sandbox(App* app, SceneManager* mgr)
    : Scene_Level(app, mgr, SCENE_SANDBOX, "")
{
}

void Scene_Sandbox::OnEnter()
{
    app->engine->paused = false;
    app->engine->stepOnce = false;
    app->engine->levelCellsProtection = false;
    requestRescan = true;

    levelFinished = false;
    levelResultSaved = false;
    levelStarsEarned = 0;
    bonusStarEarned = false;
    budgetStarEarned = false;

    app->ResetCamera();
}

void Scene_Sandbox::Update(float)
{
    if (app->input->KeyDown(GLFW_KEY_ESCAPE)) mgr->Request(SCENE_MAINMENU);

    if (requestRescan) {
        ScanLevels();
        requestRescan = false;
    }
}

void Scene_Sandbox::DrawUI(int& brushSize, Material& brushMat)
{
    app->ui->Draw(brushSize, brushMat);

    UI* ui = app->ui;
    float x = 8.0f;
    float y = 44.0f;
    float btn = 28.0f;

    auto btn3 = [&](uint32 c, const char* label) {
        uint32 h = MulRGBA(c, 1.15f), a = MulRGBA(c, 0.85f);
        float bx = x;
        bool clicked = ui->Button(bx, y, btn, btn, c, h, a);
        ui->TextCentered(bx, y, btn, btn, label, RGBAu32(250, 250, 250, 240), 0.85f);
        x += btn + 6.0f;
        return clicked;
    };

    // Back
    const float bxBack = (float)app->framebufferSize.x - 36.0f;
    if (app->ui->Button(bxBack, 8.0f, 28.0f, 28.0f, RGBAu32(200, 80, 80, 230), RGBAu32(240, 120, 120, 240), RGBAu32(170, 60, 60, 230))) {
        mgr->Request(SCENE_MAINMENU);
    }
    app->ui->TextCentered(bxBack, 8.0f, 28.0f, 28.0f, "<", RGBAu32(250, 250, 250, 240), 0.85f);

    // Settings
    const float bxSet = bxBack - 34.0f;
    uint32 cSet = RGBAu32(70, 90, 140, 220);
    if (app->ui->Button(bxSet, 8.0f, 28.0f, 28.0f, cSet, MulRGBA(cSet, 1.15f), MulRGBA(cSet, 0.85f))) {
        mgr->OpenSettings(SCENE_SANDBOX);
    }
    app->ui->TextCentered(bxSet, 8.0f, 28.0f, 28.0f, "S", RGBAu32(250, 250, 250, 240), 0.85f);

    //Load
    if (btn3(RGBAu32(80, 120, 80, 230), "L")) {
        if (!files.empty()) app->engine->LoadLevel(files[selected].c_str());
    }
    //Save
    if (btn3(RGBAu32(150, 120, 70, 230), "S")) {
        if (!files.empty()) app->engine->SaveLevel(files[selected].c_str());
    }

    //New
    if (btn3(RGBAu32(120, 90, 150, 230), "N")) {
        std::string name = NextAutoName();
        EnsureLevelsFolder();
        app->engine->ClearWorld();
        app->engine->SetLevelMaterialLimits(0, 0);
        app->engine->SaveLevel(name.c_str());
        requestRescan = true;
    }

    //Delete
    if (btn3(RGBAu32(220, 90, 90, 230), "D")) {
        //TODO
    }

    if (!files.empty()) {
        if ((int)files.size() == selected) {
            selected--;
        }
        std::string show = std::filesystem::path(files[selected]).filename().string();
        if ((int)show.size() > 28) show = show.substr(0, 25) + "...";
        ui->Text(148.0f, y + 10, show.c_str(), RGBAu32(240, 240, 240, 220), 0.85f);
    }

    // Archivos
    float gx = 8.0f;
    float gy = 80.0f;
    float cell = 20.0f;
    float pad = 4.0f;
    int perRow = (int)std::fmax(1.0f, std::floor((app->framebufferSize.x - gx) / (cell + pad)));

    for (int i = 0; i < (int)files.size(); ++i) {
        int row = i / perRow;
        int col = i % perRow;
        float bx = gx + col * (cell + pad);
        float by = gy + row * (cell + pad);

        uint32 c = ColorForName(files[i]);
        if (i == selected) c = MulRGBA(c, 1.3f);
        uint32 h = MulRGBA(c, 1.15f), a = MulRGBA(c, 0.85f);

        if (ui->Button(bx, by, cell, cell, c, h, a)) {
            selected = i;
        }

        char ibuf[8];
        auto pos = files[i].find("quick.lvl");
        if (pos == std::string::npos) {
            std::snprintf(ibuf, sizeof(ibuf), "%d", i + 1);
        }
        else {
            std::snprintf(ibuf, sizeof(ibuf), "%s", "Q");
        }

        ui->TextCentered(bx, by, cell, cell, ibuf, RGBAu32(250, 250, 250, 220), 0.65f);

        if (i == selected) {
            ui->RectBorders(bx, by, cell, cell, 2.0f, RGBAu32(240, 240, 240, 180));
        }
    }

    //Sliders Grid + level rules
    const float sx = 600.0f + 36.0f;
    const float sw = 1200.0f - 44.0f;
    const float sh = 16.0f;
    float sW = (float)app->gridSize.x;
    float sH = (float)app->gridSize.y;

    ui->Text(610.0f, 58.0f, "W", RGBAu32(235, 235, 235, 230), 0.9f);
    ui->Text(610.0f, 80.0f, "H", RGBAu32(235, 235, 235, 230), 0.9f);

    ui->Slider(sx, 56.0f, sw, sh, 64.0f, 2048.0f, sW, RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));
    ui->Slider(sx, 78.0f, sw, sh, 64.0f, 1024.0f, sH, RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));

    const int newW = std::fmax(1, (int)(sW + 0.5f));
    const int newH = std::fmax(1, (int)(sH + 0.5f));

    if (newW != app->gridSize.x || newH != app->gridSize.y) {
        app->engine->ResizeGrid(newW, newH, true);
    }

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d", app->gridSize.x);
    ui->Text(660.0f, 58.0f, buf, RGBAu32(235, 235, 235, 230), 0.9f);
    std::snprintf(buf, sizeof(buf), "%d", app->gridSize.y);
    ui->Text(660.0f, 80.0f, buf, RGBAu32(235, 235, 235, 230), 0.9f);

    const float budgetMaxRange = (float)std::fmax(100, app->gridSize.x * app->gridSize.y);
    float budgetMax = (float)app->engine->LevelMaterialBudgetMax();
    float budgetStar = (float)app->engine->LevelMaterialBudgetStar();
    if (budgetStar > budgetMax) budgetStar = budgetMax;

    ui->Text(610.0f, 108.0f, "MAT MAX", RGBAu32(235, 235, 235, 230), 0.85f);
    ui->Text(610.0f, 130.0f, "MAT STAR", RGBAu32(235, 235, 235, 230), 0.85f);
    ui->Slider(sx, 106.0f, sw, sh, 0.0f, budgetMaxRange, budgetMax, RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));
    ui->Slider(sx, 128.0f, sw, sh, 0.0f, std::fmax(0.0f, budgetMax), budgetStar, RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));

    const int newBudgetMax = std::fmax(0, (int)(budgetMax + 0.5f));
    const int newBudgetStar = std::fmax(0, (int)(budgetStar + 0.5f));
    if (newBudgetMax != app->engine->LevelMaterialBudgetMax() || newBudgetStar != app->engine->LevelMaterialBudgetStar()) {
        app->engine->SetLevelMaterialLimits(newBudgetMax, newBudgetStar);
    }

    std::snprintf(buf, sizeof(buf), "%d", app->engine->LevelMaterialBudgetMax());
    ui->Text(690.0f, 108.0f, buf, RGBAu32(235, 235, 235, 230), 0.85f);
    std::snprintf(buf, sizeof(buf), "%d", app->engine->LevelMaterialBudgetStar());
    ui->Text(690.0f, 130.0f, buf, RGBAu32(235, 235, 235, 230), 0.85f);

    //Materiales
    pad = 8.0f; y = 8.0f; x = 8.0f; btn = 28.0f;
    auto makeBtnColor = [&](uint32 base) {
        uint32 h = MulRGBA(base, 1.15f), a = MulRGBA(base, 0.85f);
        bool clicked = ui->Button(x, y, btn, btn, base, h, a);
        x += btn + 6.0f; return clicked;
    };

    for (int i = 0; i < 256; ++i) {
        const MatProps& mp = matProps((uint8)i);

        if (mp.name.length() > 0) {
            uint32 c = RGBAu32(mp.color.r, mp.color.g, mp.color.b, 230);
            if (makeBtnColor(c)) brushMat = (Material)i;
        }
    }

    x += 8.0f;

    if (app->engine->paused) {
        if (makeBtnColor(RGBAu32(250, 200, 200, 230))) app->engine->paused = false;
        if (makeBtnColor(RGBAu32(180, 220, 180, 230))) app->engine->stepOnce = true;
    }
    else {
        if (makeBtnColor(RGBAu32(200, 200, 200, 230))) app->engine->paused = true;
    }
}

void Scene_Sandbox::EnsureLevelsFolder()
{
    std::filesystem::create_directories("levels");
    std::filesystem::create_directories("levels/custom");
}

void Scene_Sandbox::ScanLevels()
{
    EnsureLevelsFolder();

    files.clear();
    for (const auto& e : std::filesystem::directory_iterator("levels/custom")) {
        if (!e.is_regular_file()) continue;
        std::filesystem::path p = e.path();
        if (p.extension() == ".lvl") {
            files.push_back(p.string());
        }
    }

    std::sort(files.begin(), files.end());

    if (files.empty()) {
        files.push_back("levels/custom/quick.lvl");
    }

    selected = std::clamp(selected, 0, (int)files.size() - 1);
}

std::string Scene_Sandbox::NextAutoName()
{
    int maxId = 0;
    for (const auto& f : files) {
        auto pos = f.find("custom_");
        if (pos == std::string::npos) continue;
        int id = std::atoi(f.c_str() + pos + 7);
        if (id > maxId) maxId = id;
        selected = id;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "levels/custom/custom_%03d.lvl", maxId + 1);
    return std::string(buf);
}
