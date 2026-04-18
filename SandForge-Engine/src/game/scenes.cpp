#include "scenes.h"

#include "scene_manager.h"

#include "app/app.h"
#include "core/engine.h"
#include "core/input.h"
#include "ui/ui.h"
#include "render/atlas.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <chrono>
#include <ctime>

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

static int CountFilledCells(const uint8* plane, int count)
{
    int total = 0;
    for (int i = 0; i < count; ++i) {
        if (plane[i] != (uint8)Material::Empty) ++total;
    }
    return total;
}

static int gSandboxMatCounterBase = 0;
static bool gSandboxResetMatCounter = true;

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
    float bw = 260.0f;
    float bh = 70.0f;
    float spacing = 8.0f;
    float cx = (vw - bw) * 0.5f;
    float cy = logoY + logoH + 28.0f;

    //if (ui->Button(cx, cy, bw, bh, base, hover, act)) mgr->Request(SCENE_LEVELSELECTOR);
    if(ui->ImageButton(ui->interfaceTex, cx, cy, bw, bh, ui->buttonUp, ui->buttonDown, ui->buttonDown, RGBAu32(180, 180, 220, 220)))
        mgr->Request(SCENE_LEVELSELECTOR);
    ui->TextCentered(cx+2, cy-2, bw, bh, "LEVELS", RGBAu32(240, 240, 240, 230), 1.0f);

    cy += bh + spacing;
    if (ui->ImageButton(ui->interfaceTex, cx, cy, bw, bh, ui->buttonUp, ui->buttonDown, ui->buttonDown, RGBAu32(170, 220, 170, 220)))
        mgr->Request(SCENE_SANDBOX);
    ui->TextCentered(cx+2, cy-2, bw, bh, "SANDBOX", RGBAu32(240, 240, 240, 230), 1.0f);

    cy += bh + spacing;
    if (ui->ImageButton(ui->interfaceTex, cx, cy, bw, bh, ui->buttonUp, ui->buttonDown, ui->buttonDown, RGBAu32(102, 161, 255, 220)))
        mgr->OpenSettings(SCENE_MAINMENU);
    ui->TextCentered(cx+2, cy-2, bw, bh, "SETTINGS", RGBAu32(240, 240, 240, 230), 1.0f);

    cy += bh + spacing;
    if (ui->ImageButton(ui->interfaceTex, cx, cy, bw, bh, ui->buttonUp, ui->buttonDown, ui->buttonDown, RGBAu32(220, 170, 170, 220)))
        app->RequestQuit();
    ui->TextCentered(cx+2, cy-2, bw, bh, "QUIT", RGBAu32(240, 240, 240, 230), 1.0f);
}

int Scene_Level::LevelIndex() const
{
    return (int)id - (int)SCENE_LEVEL1;
}

std::vector<Material> Scene_Level::GetSceneMaterials()
{
    switch (id)
    {
    case SCENE_LEVEL1:
        return std::vector<Material>{ Material::Empty, Material::Sand };
        break;
    case SCENE_LEVEL2:
        return std::vector<Material>{  Material::Empty, Material::Sand, Material::Water};
        break;
    case SCENE_LEVEL3:
        return std::vector<Material>{  Material::Empty, Material::Sand };
        break;
    case SCENE_LEVEL4:
        return std::vector<Material>{  Material::Empty, Material::Sand, Material::Steam };
        break;
    case SCENE_LEVEL5:
        return std::vector<Material>{  Material::Empty, Material::Sand, Material::Water, Material::Wood, Material::Steam };  
        break;
    case SCENE_LEVEL6:
        return std::vector<Material>{  Material::Empty, Material::Sand, Material::Fire };
        break; 
    case SCENE_LEVEL7:
        return std::vector<Material>{  Material::Empty, Material::Water, Material::Steam, Material::Fire };
        break;
    case SCENE_LEVEL8:
        return std::vector<Material>{  Material::Empty, Material::Sand, Material::Water, Material::Fire, Material::Snow };
        break;
    case SCENE_LEVEL9:
        return std::vector<Material>{  Material::Empty, Material::Sand, Material::Vine, Material::Coal, Material::FlammableGas };
        break;
    case SCENE_LEVEL10:
        return std::vector<Material>{  Material::Empty, Material::Water, Material::Vine, Material::Snow, Material::Coal };
        break;
    case SCENE_LEVEL11:
        return std::vector<Material>{  Material::Empty, Material::Sand };
        break;
    case SCENE_LEVEL12:
        return std::vector<Material>{  Material::Empty, Material::Sand };
        break;
    case SCENE_LEVEL13:
        return std::vector<Material>{  Material::Empty, Material::Sand };
        break;
    case SCENE_LEVEL14:
        return std::vector<Material>{  Material::Empty, Material::Sand };
        break;
    case SCENE_LEVEL15:
        return std::vector<Material>{  Material::Empty, Material::Sand };
        break;
    default:
        break;
    }

    return std::vector<Material>();
}

void Scene_Level::OnEnter()
{
    app->engine->paused = false;
    app->engine->stepOnce = false;
    app->engine->simSpeed = 1;
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

    app->engine->brushMat = GetSceneMaterials()[0];

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

    const float barX = ((float)app->framebufferSize.x / 2) - 480;
    const float barY = app->framebufferSize.y - 110.0f;
    const float barW = 201;
    const float barH = 34;

    //Background
    ui->Image(ui->interfaceTex, barX, barY, barW, barH, ui->hudBudgedBackgroundRect, RGBAu32(255, 255, 255, 255), 4);

    //Fill
    const float fillW = barW * fill;
    if (fillW > 0.0f) {
        ui->Image(ui->interfaceTex, barX + (barW - fillW), barY, fillW, barH, ui->hudBudgedFillRect,
            budgetStarEarned || (!levelFinished && used <= starLimit) ? RGBAu32(235, 235, 235, 245) : RGBAu32(235, 75, 75, 245), 4);
    }


    //Mark
    const float markX = barX + barW - (barW * starFill);
    ui->Image(ui->interfaceTex, markX-28, barY-50, 48, 96, ui->hudBudgedStarHolderRect, RGBAu32(255,255,255,255), 5);

    //StarMark
    ui->Image(ui->interfaceTex, markX - 15, barY - 40, 22, 22, (used <= starLimit) ? ui->starIconRect : ui->starEmptyIconRect, RGBAu32(255, 255, 255, 255), 5);

    /*char buf[64];
    std::snprintf(buf, sizeof(buf), "%d/%d", app->engine->MaterialUsed(), app->engine->LevelMaterialBudgetMax());
    ui->Text(barX + (barW/2)- 30 , barY + barH + 16.0f, buf, RGBAu32(245, 245, 245, 235), 0.70f);*/

    ui->Text(barX, barY - 20.0f, "Budget", RGBAu32(245, 245, 245, 235), 0.70f);

    ui->Text(barX, barY + barH + 16.0f, levelName.c_str(), RGBAu32(245, 245, 245, 235), 0.70f);
    
    /*char buf[64];
    std::snprintf(buf, sizeof(buf), "%d/%d", app->engine->MaterialUsed(), app->engine->LevelMaterialBudgetMax());
    ui->Text(barX + 28.0f, barY + barH - 16.0f, buf, RGBAu32(245, 245, 245, 235), 0.70f);
    ui->Text(barX - 2.0f, barY - 18.0f, "MAT", RGBAu32(245, 245, 245, 220), 0.70f);
    std::snprintf(buf, sizeof(buf), "%d", app->engine->LevelMaterialBudgetStar());*/
    //ui->Text(barX + 28.0f, markY - 8.0f, buf, RGBAu32(235, 235, 235, 220), 0.65f);
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

static float KeepAspectH(float worldW, const AtlasRectPx& r)
{
    return worldW * ((float)r.h / (float)r.w);
}

void Scene_Level::DrawUI(int& brushSize, Material& brushMat)
{
    UI* ui = app->ui;

    ui->Draw(brushSize, brushMat);
    DrawMaterialBudgetBar();

    //Tutorials
    const float camW = 80;
    const float spawnW = 42;
    const float finishW = 42;
    const float bonusW = 42;
    const float moveSidesW = 42;

    const float budgetX = ((float)app->framebufferSize.x / 2) - 810;
    const float budgetY = app->framebufferSize.y - 135.0f;
    const float barW = 201;
    const float barH = 34;

    uint32 tutorialColor = RGBAu32(255, 255, 255, 055);


    //Level 1
    if (id == SceneId::SCENE_LEVEL1) {
        ui->ImageWorld(ui->tutorialsTex, 160-40, 10, camW, KeepAspectH(camW, ui->cameraTutorialRect), ui->cameraTutorialRect, tutorialColor);

        ui->ImageWorld(ui->tutorialsTex, 10, 46, spawnW, KeepAspectH(spawnW, ui->spawnTutorialRect), ui->spawnTutorialRect, tutorialColor);
        ui->ImageWorld(ui->tutorialsTex, 260, 46, finishW, KeepAspectH(finishW, ui->finishTutorialRect), ui->spawnTutorialRect, tutorialColor);

        ui->ImageWorld(ui->tutorialsTex, 165, 90, bonusW, KeepAspectH(bonusW, ui->bonusTutorialRect), ui->bonusTutorialRect, tutorialColor);

        ui->Image(ui->tutorialsTex, budgetX, budgetY, ui->budgetTutorialRect.w * 0.8f, ui->budgetTutorialRect.h*0.8f, ui->budgetTutorialRect, tutorialColor);
    }

    //Level 5
    if (id == SceneId::SCENE_LEVEL5) {
        ui->ImageWorld(ui->tutorialsTex, 70, 50, moveSidesW, KeepAspectH(moveSidesW, ui->moveSidesTutorialRect), ui->moveSidesTutorialRect, tutorialColor);
    }

    //Level 2
    if (id == SceneId::SCENE_LEVEL6) {
        ui->ImageWorld(ui->tutorialsTex, 110, 200, moveSidesW, KeepAspectH(moveSidesW, ui->moveUpTutorialRect), ui->moveUpTutorialRect, tutorialColor);
    }
    


    //background
    ui->Image(ui->interfaceTex, 0, app->framebufferSize.y - 150, app->framebufferSize.x, 150, ui->hudBackgroundRect, RGBAu32(255, 255, 255, 255), -10);


    //SLIDER
    float y = app->framebufferSize.y - 50.0f;
    float x = ((float)app->framebufferSize.x / 2) - 110;
    float v = (float)brushSize;

    app->ui->Slider(x, y, 220, 20.0f, 1.0f, 64.0f, v, true);
    brushSize = (int)(v + 0.5f);
    
    float s = 28.0f;

    //Down Materials
    {
        std::vector<Material> levelsMaterials = GetSceneMaterials();

        int materialCount = levelsMaterials.size();

        //app->ui->Rect(app->framebufferSize.x / 2.0f, 0, 1, app->framebufferSize.y, RGBAu32(255,255,255,255));

        x = ((float)app->framebufferSize.x / 2) - (((44.5* materialCount)) /2);
        x += 20;
        y = app->framebufferSize.y - 100.0f;

        app->ui->Rect(0, app->framebufferSize.y - 150, app->framebufferSize.x, 150, RGBAu32(24, 24, 24, 255));

        auto makeBtnColor = [&](Material mat) {
            //Background
            bool clicked = false;
            if (app->engine->brushMat == mat) {
                clicked = app->ui->ImageButton(app->ui->interfaceTex, x-27.5, y-27.5, 55, 55, ui->hudMaterialBackgroundSelectedRect, RGBAu32(255,255,255,255), RGBAu32(255, 255, 255, 155), RGBAu32(255, 255, 255, 255), 4);
            }
            else {
                clicked = app->ui->ImageButton(app->ui->interfaceTex, x-21, y-21, 42, 42, ui->hudMaterialBackgroundRect, RGBAu32(255, 255, 255, 255), RGBAu32(255, 255, 255, 155), RGBAu32(255, 255, 255, 255), 4);
            }
            app->ui->Image(app->ui->matAtlas, x-16, y-16, 32, 32, matProps(mat).rect, RGBAu32(255, 255, 255, 255), 5);
            x += 50;

            return clicked; 
        };

        for (int i = 0; i < materialCount; ++i) {
            if (makeBtnColor(levelsMaterials[i])) brushMat = levelsMaterials[i];
        }

        const float controlsCenterX = ((float)app->framebufferSize.x / 2) + 380.0f;
        const float controlY = y - 21.0f;
        const float controlGap = 8.0f;
        const float controlBgSize = 42.0f;
        const float controlIconSize = 32.0f;
        const float controlsWidth = controlBgSize * 3.0f + controlGap * 2.0f;
        const float controlsStartX = controlsCenterX - controlsWidth * 0.5f;

        auto drawSpeedButton = [&](float bx, float by) {
            uint32 tint = RGBAu32(255, 255, 255, 255);
            if (app->engine->simSpeed == 2) tint = RGBAu32(210, 235, 255, 255);
            else if (app->engine->simSpeed == 4) tint = RGBAu32(170, 225, 255, 255);

            app->ui->Image(app->ui->interfaceTex, bx, by, controlBgSize, controlBgSize, ui->hudMaterialBackgroundRect, tint, 4);

            if (app->ui->Button(bx + 5, by + 5, controlIconSize, controlIconSize,
                RGBAu32(0, 0, 0, 0), RGBAu32(255, 255, 255, 30), RGBAu32(255, 255, 255, 55))) {
                app->engine->CycleSimSpeed();
            }

            char speedLabel[8];
            std::snprintf(speedLabel, sizeof(speedLabel), "x%d", app->engine->simSpeed);
            app->ui->TextCentered(bx, by + 1, controlBgSize, controlBgSize, speedLabel, RGBAu32(250, 250, 250, 240), 0.72f);
        };

        const float speedBgX = controlsStartX;
        const float pauseBgX = speedBgX + controlBgSize + controlGap;
        const float stepBgX = pauseBgX + controlBgSize + controlGap;
        const float pauseIconX = pauseBgX + (controlBgSize - controlIconSize) * 0.5f;
        const float stepIconX = stepBgX + (controlBgSize - controlIconSize) * 0.5f;
        const float controlIconY = y - 16.0f;

        if (app->engine->paused && !levelFinished) {

            drawSpeedButton(speedBgX, controlY);

            //Pausa
            app->ui->Image(app->ui->interfaceTex, pauseBgX, controlY, controlBgSize, controlBgSize, ui->hudMaterialBackgroundRect, RGBAu32(255, 255, 255, 255), 4);
            if(app->ui->ImageButton(app->ui->interfaceTex, pauseIconX, controlIconY, controlIconSize, controlIconSize, ui->playIcon, RGBAu32(255, 255, 255, 255), RGBAu32(255, 255, 255, 200), RGBAu32(255, 255, 255, 55), 5))  app->engine->paused = false;

            //Step
            app->ui->Image(app->ui->interfaceTex, stepBgX, controlY, controlBgSize, controlBgSize, ui->hudMaterialBackgroundRect, RGBAu32(255, 255, 255, 255), 4);
            if (app->ui->ImageButton(app->ui->interfaceTex, stepIconX, controlIconY, controlIconSize, controlIconSize, ui->stepIcon, RGBAu32(255, 255, 255, 255), RGBAu32(255, 255, 255, 200), RGBAu32(255, 255, 255, 55), 5)) app->engine->stepOnce = true;
        }
        else if (!levelFinished) {

            drawSpeedButton(speedBgX, controlY);

            app->ui->Image(app->ui->interfaceTex, pauseBgX, controlY, controlBgSize, controlBgSize, ui->hudMaterialBackgroundRect, RGBAu32(255, 255, 255, 255), 4);
            if (app->ui->ImageButton(app->ui->interfaceTex, pauseIconX, controlIconY, controlIconSize, controlIconSize, ui->pauseIcon, RGBAu32(255, 255, 255, 255), RGBAu32(255, 255, 255, 200), RGBAu32(255, 255, 255, 55), 5))  app->engine->paused = true;

            app->ui->Image(app->ui->interfaceTex, stepBgX, controlY, controlBgSize, controlBgSize, ui->hudMaterialBackgroundRect, RGBAu32(255, 255, 255, 255), 4);
            app->ui->Image(app->ui->interfaceTex, stepIconX, controlIconY, controlIconSize, controlIconSize, ui->stepIcon, RGBAu32(255, 255, 255, 055), 5);
        }


        //Timer:
        app->ui->TextCentered(controlsCenterX - s * 0.5f, y+50, s, s, app->engine->sceneTimer.ReadString().c_str(), RGBAu32(250, 250, 250, 240), 1.2f);
    }

    //Separadores
    {
        x = ((float)app->framebufferSize.x / 2);
        y = app->framebufferSize.y - 164.0f;
        //app->ui->Image(app->ui->interfaceTex, x - 19.5, y, 39, 192, AtlasRectPx{ 5,677, 39, 192 }, RGBAu32(255, 255, 255, 255), 6);


        app->ui->Image(app->ui->interfaceTex, x - 520-19.5, y, 39, 192, ui->hudSeparatorWoodRect, RGBAu32(255, 255, 255, 255), 6);
        app->ui->Image(app->ui->interfaceTex, x-240 - 19.5, y, 39, 192, ui->hudSeparatorWoodRect, RGBAu32(255, 255, 255, 255), 6);
        app->ui->Image(app->ui->interfaceTex, x+240 - 19.5, y, 39, 192, ui->hudSeparatorWoodRect, RGBAu32(255, 255, 255, 255), 6);
        app->ui->Image(app->ui->interfaceTex, x+520 - 19.5, y, 39, 192, ui->hudSeparatorWoodRect, RGBAu32(255, 255, 255, 255), 6);
    
    }

    //Back
    x = (float)app->framebufferSize.x - 60.0f;
    y = 8.0f;
    s = 48;
    if (ui->ImageButton(ui->interfaceTex, x, y, s, s, ui->buttonLittleUp, ui->buttonLittleDown, ui->buttonLittleDown, RGBAu32(220, 170, 170, 220))) {
        mgr->Request(SCENE_LEVELSELECTOR);
    }
    app->ui->TextCentered(x+3, y-2, s, s, "<", RGBAu32(250, 250, 250, 240), 0.85f);

    //Settings
    x -= 46.0f;
    if (ui->ImageButton(ui->interfaceTex, x, y, s, s, ui->buttonLittleUp, ui->buttonLittleDown, ui->buttonLittleDown, RGBAu32(102, 161, 255, 220))) {
        mgr->OpenSettings(GetId());
    }
    app->ui->TextCentered(x+4, y-2, s, s, "S", RGBAu32(250, 250, 250, 240), 0.85f);

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
    app->engine->simSpeed = 1;
    app->engine->levelCellsProtection = false;
    requestRescan = true;

    levelFinished = false;
    levelResultSaved = false;
    levelStarsEarned = 0;
    bonusStarEarned = false;
    budgetStarEarned = false;

    gSandboxResetMatCounter = true;

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
    UI* ui = app->ui;
    const float vw = (float)app->framebufferSize.x;

    ui->Draw(brushSize, brushMat);
    ui->Cursor();

    static bool showGridPanel = false;

    const int totalMat = CountFilledCells(app->engine->GetFrontPlane(), app->gridSize.x * app->gridSize.y);
    if (gSandboxResetMatCounter) {
        gSandboxMatCounterBase = totalMat;
        gSandboxResetMatCounter = false;
    }
    const int matCounter = totalMat - gSandboxMatCounterBase;

    const uint32 text = RGBAu32(245, 245, 245, 235);
    const uint32 sub = RGBAu32(205, 210, 220, 220);
    const uint32 bg = RGBAu32(16, 20, 28, 185);
    const uint32 bg2 = RGBAu32(24, 30, 40, 220);
    const uint32 line = RGBAu32(255, 255, 255, 36);

    auto tinyBtn = [&](float x, float y, float w, float h, uint32 c, const char* label, float scale = 0.72f) {
        bool clicked = ui->Button(x, y, w, h, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f));
        ui->TextCentered(x, y, w, h, label, RGBAu32(250, 250, 250, 240), scale);
        return clicked;
        };

    auto presetBtn = [&](float x, float y, float w, float h, int current, int target, const char* label, uint32 c) {
        uint32 cc = (current == target) ? MulRGBA(c, 1.20f) : c;
        bool clicked = ui->Button(x, y, w, h, cc, MulRGBA(cc, 1.10f), MulRGBA(cc, 0.85f));
        ui->TextCentered(x, y, w, h, label, RGBAu32(255, 255, 255, 240), 0.68f);
        if (current == target) {
            ui->RectBorders(x, y, w, h, 2.0f, RGBAu32(255, 255, 255, 90));
        }
        return clicked;
        };

    // Clamp selección
    if (!files.empty()) {
        if (selected < 0) selected = 0;
        if (selected >= (int)files.size()) selected = (int)files.size() - 1;
    }
    else {
        selected = 0;
    }

    // Botones arriba derecha
    {
        const float b = 28.0f;
        const float speedW = 36.0f;
        const float by = 8.0f;
        const float bxBack = vw - 8.0f - b;
        const float bxSet = bxBack - 34.0f;
        const float bxGrid = bxSet - 34.0f;
        const float bxSpeed = bxGrid - 6.0f - speedW;

        char speedLabel[8];
        std::snprintf(speedLabel, sizeof(speedLabel), "x%d", app->engine->simSpeed);
        uint32 speedC = RGBAu32(84, 104, 160, 220);
        if (app->engine->simSpeed == 2) speedC = RGBAu32(110, 145, 210, 230);
        else if (app->engine->simSpeed == 4) speedC = RGBAu32(90, 180, 230, 235);
        if (tinyBtn(bxSpeed, by, speedW, b, speedC, speedLabel, 0.62f)) {
            app->engine->CycleSimSpeed();
        }

        // Back
        if (tinyBtn(bxBack, by, b, b, RGBAu32(200, 80, 80, 230), "<", 0.85f)) {
            mgr->Request(SCENE_MAINMENU);
        }

        // Settings
        if (tinyBtn(bxSet, by, b, b, RGBAu32(70, 90, 140, 220), "S", 0.80f)) {
            mgr->OpenSettings(SCENE_SANDBOX);
        }

        uint32 cGrid = showGridPanel ? RGBAu32(120, 110, 190, 235) : RGBAu32(70, 70, 90, 220);
        if (tinyBtn(bxGrid, by, b, b, cGrid, "G", 0.80f)) {
            showGridPanel = !showGridPanel;
        }
    }

    // Panel nivel
    {
        const float x0 = 8.0f;
        const float y0 = 8.0f;
        const float w = 330.0f;

        const float cell = 18.0f;
        const float pad = 4.0f;
        const int perRow = 8;
        const int rows = std::fmax(1, ((int)files.size() + perRow - 1) / perRow);
        const float h = std::fmax(62.0f, 54.0f + rows * (cell + pad));

        ui->Rect(x0, y0, w, h, bg);
        ui->RectBorders(x0, y0, w, h, 1.0f, line);

        const float by = y0 + 6.0f;
        const float bw = 46.0f;
        const float bh = 22.0f;
        const float gap = 4.0f;

        //Load
        if (tinyBtn(x0 + 6.0f + (bw + gap) * 0, by, bw, bh, RGBAu32(80, 120, 80, 230), "LOAD", 0.58f)) {
            if (!files.empty()) {
                app->engine->LoadLevel(files[selected].c_str());
                gSandboxResetMatCounter = true;
            }
        }

        //Save
        if (tinyBtn(x0 + 6.0f + (bw + gap) * 1, by, bw, bh, RGBAu32(150, 120, 70, 230), "SAVE", 0.58f)) {
            if (!files.empty()) app->engine->SaveLevel(files[selected].c_str());
        }

        //New
        if (tinyBtn(x0 + 6.0f + (bw + gap) * 2, by, bw, bh, RGBAu32(120, 90, 150, 230), "NEW", 0.60f)) {
            std::string name = NextAutoName();
            EnsureLevelsFolder();
            app->engine->ClearWorld();
            app->engine->SetLevelMaterialLimits(0, 0);
            app->engine->SaveLevel(name.c_str());
            requestRescan = true;
            gSandboxResetMatCounter = true;
        }

        //Delete
        if (tinyBtn(x0 + 6.0f + (bw + gap) * 3, by, bw, bh, RGBAu32(220, 90, 90, 230), "DEL", 0.60f)) {
            if (!files.empty() && selected >= 0 && selected < (int)files.size()) {
                std::error_code ec;
                std::filesystem::remove(files[selected], ec);
                requestRescan = true;
                if (selected > 0) selected--;
            }
        }

        //Files
        if (!files.empty()) {
            std::string show = std::filesystem::path(files[selected]).filename().string();
            if ((int)show.size() > 24) show = show.substr(0, 21) + "...";
            ui->Text(x0 + 6.0f, y0 + 36.0f, show.c_str(), text, 0.72f);
        }
        else {
            ui->Text(x0 + 6.0f, y0 + 36.0f, "no levels", sub, 0.72f);
        }

        const float fx = x0 + 6.0f;
        const float fy = y0 + 54.0f;

        for (int i = 0; i < (int)files.size(); ++i) {
            int row = i / perRow;
            int col = i % perRow;

            float bx = fx + col * (cell + pad);
            float by2 = fy + row * (cell + pad);

            uint32 c = ColorForName(files[i]);
            if (i == selected) c = MulRGBA(c, 1.25f);

            if (ui->Button(bx, by2, cell, cell, c, MulRGBA(c, 1.15f), MulRGBA(c, 0.85f))) {
                selected = i;
            }

            char ibuf[8];
            auto pos = files[i].find("quick.lvl");
            if (pos == std::string::npos) std::snprintf(ibuf, sizeof(ibuf), "%d", i + 1);
            else                          std::snprintf(ibuf, sizeof(ibuf), "Q");

            ui->TextCentered(bx, by2, cell, cell, ibuf, RGBAu32(250, 250, 250, 230), 0.58f);

            if (i == selected) {
                ui->RectBorders(bx, by2, cell, cell, 2.0f, RGBAu32(255, 255, 255, 180));
            }
        }
    }

    //Sliders Grid + level rules
    if (showGridPanel)
    {
        float sW = (float)app->gridSize.x;
        float sH = (float)app->gridSize.y;

        const float panelW = std::fmin(1420.0f, vw - 16.0f);
        const float panelH = 190.0f;
        const float x0 = vw - 8.0f - panelW;
        const float y0 = 44.0f;

        ui->Rect(x0, y0, panelW, panelH, bg2);
        ui->RectBorders(x0, y0, panelW, panelH, 1.0f, line);

        const float statsW = 180.0f;
        const float statsX = x0 + panelW - statsW - 12.0f;

        const float left = x0 + 12.0f;
        const float sliderX = left + 26.0f;
        const float sliderW = statsX - sliderX - 56.0f;

        const float pW = 80.0f;
        const float pH = 20.0f;
        const float pGap = 8.0f;

        ui->Text(left, y0 + 10.0f, "W", text, 0.74f);
        if (presetBtn(left + 26.0f + (pW + pGap) * 0, y0 + 8.0f, pW, pH, (int)sW, 320, "320", RGBAu32(84, 104, 160, 220))) sW = 320.0f;
        if (presetBtn(left + 26.0f + (pW + pGap) * 1, y0 + 8.0f, pW, pH, (int)sW, 480, "480", RGBAu32(84, 104, 160, 220))) sW = 480.0f;
        if (presetBtn(left + 26.0f + (pW + pGap) * 2, y0 + 8.0f, pW, pH, (int)sW, 640, "640", RGBAu32(84, 104, 160, 220))) sW = 640.0f;

        ui->Text(left, y0 + 40.0f, "W", sub, 0.72f);
        ui->Slider(sliderX, y0 + 38.0f, sliderW, 18.0f, 64.0f, 2048.0f, sW,
            RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d", (int)(sW + 0.5f));
        ui->Text(sliderX + sliderW + 8.0f, y0 + 40.0f, buf, text, 0.72f);

        ui->Text(left, y0 + 72.0f, "H", text, 0.74f);
        if (presetBtn(left + 26.0f + (pW + pGap) * 0, y0 + 70.0f, pW, pH, (int)sH, 160, "160", RGBAu32(100, 92, 152, 220))) sH = 160.0f;
        if (presetBtn(left + 26.0f + (pW + pGap) * 1, y0 + 70.0f, pW, pH, (int)sH, 240, "240", RGBAu32(100, 92, 152, 220))) sH = 240.0f;
        if (presetBtn(left + 26.0f + (pW + pGap) * 2, y0 + 70.0f, pW, pH, (int)sH, 320, "320", RGBAu32(100, 92, 152, 220))) sH = 320.0f;

        ui->Text(left, y0 + 102.0f, "H", sub, 0.72f);
        ui->Slider(sliderX, y0 + 100.0f, sliderW, 18.0f, 64.0f, 1024.0f, sH,
            RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));

        std::snprintf(buf, sizeof(buf), "%d", (int)(sH + 0.5f));
        ui->Text(sliderX + sliderW + 8.0f, y0 + 102.0f, buf, text, 0.72f);

        const int newW = std::fmax(1, (int)(sW + 0.5f));
        const int newH = std::fmax(1, (int)(sH + 0.5f));
        if (newW != app->gridSize.x || newH != app->gridSize.y) {
            app->engine->ResizeGrid(newW, newH, true);
            gSandboxResetMatCounter = true;
        }

        const float budgetMaxRange = (float)std::fmax(100, app->gridSize.x * app->gridSize.y);
        float budgetMax = (float)app->engine->LevelMaterialBudgetMax();
        float budgetStar = (float)app->engine->LevelMaterialBudgetStar();
        if (budgetStar > budgetMax) budgetStar = budgetMax;

        ui->Text(left, y0 + 134.0f, "MAX", text, 0.70f);
        ui->Slider(sliderX, y0 + 132.0f, sliderW, 18.0f, 0.0f, budgetMaxRange, budgetMax,
            RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));

        std::snprintf(buf, sizeof(buf), "%d", (int)(budgetMax + 0.5f));
        ui->Text(sliderX + sliderW + 8.0f, y0 + 134.0f, buf, text, 0.70f);

        ui->Text(left, y0 + 162.0f, "STAR", text, 0.70f);
        ui->Slider(sliderX, y0 + 160.0f, sliderW, 18.0f, 0.0f, std::fmax(0.0f, budgetMax), budgetStar,
            RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));

        std::snprintf(buf, sizeof(buf), "%d", (int)(budgetStar + 0.5f));
        ui->Text(sliderX + sliderW + 8.0f, y0 + 162.0f, buf, text, 0.70f);

        const int newBudgetMax = std::fmax(0, (int)(budgetMax + 0.5f));
        const int newBudgetStar = std::fmax(0, (int)(budgetStar + 0.5f));
        if (newBudgetMax != app->engine->LevelMaterialBudgetMax() ||
            newBudgetStar != app->engine->LevelMaterialBudgetStar()) {
            app->engine->SetLevelMaterialLimits(newBudgetMax, newBudgetStar);
        }

        //Stats
        ui->Rect(statsX, y0 + 10.0f, statsW, panelH - 20.0f, bg);
        ui->RectBorders(statsX, y0 + 10.0f, statsW, panelH - 20.0f, 1.0f, line);

        ui->Text(statsX + 10.0f, y0 + 22.0f, "MAT TOTAL", text, 0.72f);
        std::snprintf(buf, sizeof(buf), "%d", totalMat);
        ui->Text(statsX + 10.0f, y0 + 44.0f, buf, RGBAu32(255, 255, 255, 240), 1.0f);

        ui->Text(statsX + 10.0f, y0 + 80.0f, "MAT COUNT", text, 0.72f);
        std::snprintf(buf, sizeof(buf), "%d", matCounter);
        ui->Text(statsX + 10.0f, y0 + 102.0f, buf, RGBAu32(255, 255, 255, 240), 1.0f);

        if (tinyBtn(statsX + 10.0f, y0 + 136.0f, statsW - 20.0f, 24.0f, RGBAu32(120, 90, 150, 230), "RESET", 0.64f)) {
            gSandboxMatCounterBase = totalMat;
        }
    }

    //Materiales
    {
        const float btn = 28.0f;
        const float icon = 32.0f;
        const float gap = 6.0f;
        const float pad = 8.0f;
        const float barH = 44.0f;

        float x = pad;
        float y = (float)app->framebufferSize.y - barH + 6.0f;

        // fondo barra inferior
        ui->Rect(0.0f, (float)app->framebufferSize.y - barH, vw, barH, bg);
        ui->RectBorders(0.0f, (float)app->framebufferSize.y - barH, vw, barH, 1.0f, line);

        auto makeBtnColor = [&](Material mat) {
            float bx = x;
            float by = y;
            const MatProps& mp = matProps((uint8)mat);
             
            bool clicked = app->ui->ImageButton(
                app->ui->matAtlas, 
                bx, by, icon, icon,
                mp.rect,
                RGBAu32(255, 255, 255, 255), RGBAu32(255, 255, 255, 155), RGBAu32(255, 255, 255, 205), 5
            );

            if ((int)brushMat == mat) {
                ui->RectBorders(bx, by, icon, icon, 2.0f, RGBAu32(255, 255, 255, 180));
            }

            x += btn + gap;
            return clicked;
            };

        for (int i = 0; i < 256; ++i) {
            if (i < Material::COUNT) {
                if (makeBtnColor((Material)i)) brushMat = (Material)i;
            }
        }
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
    using namespace std::chrono;

    auto now = system_clock::now();
    std::time_t tt = system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    char buf[64];
    std::strftime(buf, sizeof(buf), "levels/custom/custom_%Y%m%d_%H-%M-%S.lvl", &tm);

    return std::string(buf);
}