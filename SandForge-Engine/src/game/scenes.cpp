#include "scenes.h"

#include "scene_manager.h"

#include "app/app.h"
#include "core/engine.h"
#include "core/input.h"
#include "ui/ui.h"

#include <algorithm>
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

    ui->Rect(0, 0, vw, vh, RGBAu32(20, 20, 24, 255));

    // Logo
    static Texture2D sLogo;
    static bool sLogoLoaded = false;
    if (!sLogoLoaded) {
        sLogoLoaded = sLogo.Load("assets/sprites/sandforge_logo.png");
    }

    float logoW = 1063.0f;
    float logoH = 503.0f;

    float maxW = vw * 0.60f;
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

void Scene_LevelSelector::Update(float)
{
    // ESC -> menu
    if (app->input->KeyDown(GLFW_KEY_ESCAPE)) mgr->Request(SCENE_MAINMENU);
}

void Scene_LevelSelector::DrawUI(int&, Material&)
{
    UI* ui = app->ui;
    float vw = (float)app->framebufferSize.x;
    float vh = (float)app->framebufferSize.y;

    ui->Rect(0, 0, vw, vh, RGBAu32(24, 24, 28, 255));

    float bw = 240.0f;
    float bh = 48.0f;
    float cx = (vw - bw) * 0.5f;
    float cy = (vh - (bh * 5 + 16.0f * 4)) * 0.5f;

    uint32 cTut = RGBAu32(70, 90, 140, 220);
    uint32 cL1  = RGBAu32(80, 120, 80, 220);
    uint32 cL2  = RGBAu32(150, 120, 70, 220);

    if (ui->Button(cx, cy, bw, bh, cTut, MulRGBA(cTut, 1.15f), MulRGBA(cTut, 0.85f))) mgr->Request(SCENE_TUTORIAL);
	ui->TextCentered(cx, cy, bw, bh, "TUTORIAL", RGBAu32(240,240,240,230), 1.0f);
    cy += bh + 16.0f;
    if (ui->Button(cx, cy, bw, bh, cL1, MulRGBA(cL1, 1.15f), MulRGBA(cL1, 0.85f))) mgr->Request(SCENE_LEVEL1);
	ui->TextCentered(cx, cy, bw, bh, "LEVEL 1", RGBAu32(240,240,240,230), 1.0f);
    cy += bh + 16.0f;
    if (ui->Button(cx, cy, bw, bh, cL2, MulRGBA(cL2, 1.15f), MulRGBA(cL2, 0.85f))) mgr->Request(SCENE_LEVEL2);
	ui->TextCentered(cx, cy, bw, bh, "LEVEL 2", RGBAu32(240,240,240,230), 1.0f);
    cy += bh + 16.0f;

    uint32 cSet = RGBAu32(70, 90, 140, 220);
    if (ui->Button(cx, cy, bw, bh, cSet, MulRGBA(cSet, 1.15f), MulRGBA(cSet, 0.85f))) mgr->OpenSettings(SCENE_LEVELSELECTOR);
	ui->TextCentered(cx, cy, bw, bh, "SETTINGS", RGBAu32(240,240,240,230), 1.0f);
    cy += bh + 16.0f;

    if (ui->Button(cx, cy, bw, bh, RGBAu32(120, 70, 70, 220), RGBAu32(150, 90, 90, 230), RGBAu32(100, 60, 60, 220))) mgr->Request(SCENE_MAINMENU);
	ui->TextCentered(cx, cy, bw, bh, "BACK", RGBAu32(240,240,240,230), 1.0f);
}

void Scene_Level::OnEnter()
{
	app->engine->paused = false;
	app->engine->stepOnce = false;

    // Load assigned level
    if (!levelPath.empty()) {
        app->engine->LoadLevel(levelPath.c_str());
    }
    app->ResetCamera();
}

void Scene_Level::Update(float)
{
    if (app->input->KeyDown(GLFW_KEY_ESCAPE)) mgr->Request(SCENE_LEVELSELECTOR);
}

void Scene_Level::DrawUI(int& brushSize, Material& brushMat)
{
    //CAMBIAR LA UI A UNA MAS MODULAR PARA LOS NIVELES
    app->ui->Draw(brushSize, brushMat);

    //Back
    float x = (float)app->framebufferSize.x - 36.0f;
    float y = 8.0f;
    float s = 28.0f;
    if (app->ui->Button(x, y, s, s, RGBAu32(200, 80, 80, 230), RGBAu32(240, 120, 120, 240), RGBAu32(170, 60, 60, 230))) {
        mgr->Request(SCENE_LEVELSELECTOR);
    }
	app->ui->TextCentered(x, y, s, s, "<", RGBAu32(250,250,250,240), 0.85f);


    //Settings
    float sx = x - 34.0f;
    uint32 cSet = RGBAu32(70, 90, 140, 220);
    if (app->ui->Button(sx, y, s, s, cSet, MulRGBA(cSet, 1.15f), MulRGBA(cSet, 0.85f))) {
        mgr->OpenSettings(GetId());
    }
    app->ui->TextCentered(sx, y, s, s, "S", RGBAu32(250,250,250,240), 0.85f);
}

Scene_Sandbox::Scene_Sandbox(App* app, SceneManager* mgr)
    : Scene_Level(app, mgr, SCENE_SANDBOX, "")
{
}

void Scene_Sandbox::OnEnter()
{
	app->engine->paused = false;
	app->engine->stepOnce = false;
    requestRescan = true;

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
    //CAMBIAR LA UI A UNA MAS COMPLETA Y COMODA DE USAR EN EL SANDBOX
    app->ui->Draw(brushSize, brushMat);

    UI* ui = app->ui;
    float x = 8.0f;
    float y = 44.0f;
    float btn = 28.0f;

    auto btn3 = [&](uint32 c, const char* label) {
        uint32 h = MulRGBA(c, 1.15f), a = MulRGBA(c, 0.85f);
        float bx = x;
        bool clicked = ui->Button(bx, y, btn, btn, c, h, a);
		ui->TextCentered(bx, y, btn, btn, label, RGBAu32(250,250,250,240), 0.85f);
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
        app->engine->ClearWorld(); //Limpiamos el mundo (si quiero mantener que se quite esto)
        app->engine->SaveLevel(name.c_str());
        requestRescan = true;
    }

    //Delete
    if (btn3(RGBAu32(220, 90, 90, 230), "D")) {
        //TODO
    }

    if (!files.empty()) {
        if (files.size() == selected) {
            selected--;
        }
        std::string show = std::filesystem::path(files[selected]).filename().string();
        if ((int)show.size() > 28) show = show.substr(0, 25) + "...";
		ui->Text(148.0f, y + 10, show.c_str(), RGBAu32(240,240,240,220), 0.85f);
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
        
		ui->TextCentered(bx, by, cell, cell, ibuf, RGBAu32(250,250,250,220), 0.65f);

        if (i == selected) {
            ui->RectBorders(bx, by, cell, cell, 2.0f, RGBAu32(240, 240, 240, 180));
        }
    }

    //Sliders Grid
    const float sx = 600 + 36.0f;
    const float sw = 600 - 44.0f;
    const float sh = 16.0f;
    float sW = (float)app->gridSize.x;
    float sH = (float)app->gridSize.y;

    ui->Text(600 + 10.0f, 30 + 28.0f, "W", RGBAu32(235, 235, 235, 230), 0.9f);
    ui->Text(600 + 10.0f, 30 + 50.0f, "H", RGBAu32(235, 235, 235, 230), 0.9f);

    ui->Slider(sx, 30 + 26.0f, sw, sh, 64.0f, 2048.0f, sW, RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));
    ui->Slider(sx, 30 + 48.0f, sw, sh, 64.0f, 1024.0f, sH, RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));

    const int newW = std::fmax(1, (int)(sW + 0.5f));
    const int newH = std::fmax(1, (int)(sH + 0.5f));

    // Aplica solo si cambia el entero (durante drag, esto corre en el pass noRender)
    if (newW != app->gridSize.x || newH != app->gridSize.y) {
        app->engine->ResizeGrid(newW, newH, true);
    }

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", app->gridSize.x);
    ui->Text(600 + 100 - 50.0f, 30 + 28.0f, buf, RGBAu32(235, 235, 235, 230), 0.9f);
    std::snprintf(buf, sizeof(buf), "%d", app->gridSize.y);
    ui->Text(600 + 100 - 50.0f, 30 + 50.0f, buf, RGBAu32(235, 235, 235, 230), 0.9f);

    //Materiales
    pad = 8.0f, y = 8.0f, x = 8.0f, btn = 28.0f;
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
    //levels/custom_XXX.lvl
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
