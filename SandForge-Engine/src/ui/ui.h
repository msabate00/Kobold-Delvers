#pragma once
#include "app/module.h"
#include "font.h"
#include <vector>
#include <cstdint>
#include <core/material.h>
#include <render/sprite.h>
#include <render/texture.h>
#include <render/atlas.h>


struct Vertex;


class UI : public Module {
public:
    UI(App* app, bool start_enabled = true);
    virtual ~UI();

    bool Awake();
    bool Start();
    bool PreUpdate();
    bool Update(float dt);
    bool PostUpdate();

    bool CleanUp();

    void Begin(int viewW, int viewH);
    void End();
    void Draw(int& brushSize, Material& brushMat);

    void SetMouse(double x, double y, bool down);
    bool ConsumedMouse() const { return mouseConsumed; }
    void SetNoRender(bool b) { noRender = b; }


    bool Button(float x, float y, float w, float h,
        uint32 rgba, uint32 rgbaHover, uint32 rgbaActive);
    // Botón con textura (atlas) + fondo tintado (se renderiza vía SpriteBatch en el primer pass)
    bool ButtonAtlas(float x, float y, float w, float h,
        int atlasIndex, uint32 rgba, uint32 rgbaHover, uint32 rgbaActive);

    bool Slider(float x, float y, float w, float h,
        float minv, float maxv, float& v, uint32 track, uint32 knob);
    bool Slider(float x, float y, float w, float h,
        float minv, float maxv, float& v, bool fill = false,
        uint32 tint = 0xFFFFFFFF);

    void Circle(float cx, float cy, float r, uint32 c, int segments = 0);
    void Ring(float cx, float cy, float r, float t, uint32 c, int segments = 0);

    // Icons
    void Star(float cx, float cy, float r, uint32 rgba);
    void StarOutline(float cx, float cy, float r, uint32 outline, uint32 fill);


    void Rect(float x, float y, float w, float h, uint32 rgba);
    void RectBorders(float x, float y, float w, float h, float t, uint32 rgba);
    void RectBordersWorld(int x, int y, int w, int h, float thickness, uint32 color);

    void Image(const Texture2D& t, float x, float y, float w, float h, uint32 tint = 0xFFFFFFFF, int sort = 0);
    void Image(const Texture2D& t, float x, float y, float w, float h, const AtlasRectPx& srcPx, uint32 tint = 0xFFFFFFFF, int sort = 0);
    bool ImageButton(const Texture2D& t, float x, float y, float w, float h, uint32 tint_rgba, uint32 tint_rgbaHover, uint32 tint_rgbaActive, int sort = 0);
    bool ImageButton(const Texture2D& t, float x, float y, float w, float h, const AtlasRectPx& srcPx, uint32 tint_rgba, uint32 tint_rgbaHover, uint32 tint_rgbaActive, int sort = 0);
    bool ImageButton(const Texture2D& t, float x, float y, float w, float h, const AtlasRectPx& srcPx, const AtlasRectPx& srcPxHover, const AtlasRectPx& srcPxActive, uint32 tint_rgba, int sort = 0);

    void Text(float x, float y, const char* text, uint32 rgba, float scale = 1.0f);
    void TextCentered(float x, float y, float w, float h, const char* text, uint32 rgba, float scale = 1.0f);

    void Cursor();

private:

    void Flush();

    bool SliderLogic(float x, float y, float w, float h,
        float minv, float maxv, float& v,
        float& t, float& kx, float& kx0, float& kw);

    void LevelCellsProtectionMark(int brushSize);

public:
    Texture2D matAtlas;
    bool matAtlasReady = false;

    Texture2D interfaceTex;
    bool interfaceTexReady = false;


    AtlasRectPx hudBackgroundRect{0,18,1920,150};
    AtlasRectPx hudSeparatorWoodRect{6,182,39,172};

    AtlasRectPx hudMaterialBackgroundSelectedRect{5,356,54,54};
    AtlasRectPx hudMaterialBackgroundRect{66,362,42,42};

    AtlasRectPx sliderBackgroundRect{ 135,375,220,16 };
    AtlasRectPx sliderFillRect{ 413,375,220,16 };
    AtlasRectPx sliderKnobRect{ 374,362,21,42 };

    AtlasRectPx hudBudgedBackgroundRect{135,331,201,34};
    AtlasRectPx hudBudgedFillRect{135,293,201,34};
    AtlasRectPx hudBudgedStarHolderRect{64,257,48,97};

    AtlasRectPx starIconRect{ 59,183,56,59 };
    AtlasRectPx starEmptyIconRect{ 116,183,56,59 };

    AtlasRectPx lockIconRect{ 654,200,151,202 };
    AtlasRectPx panelLevelRect{ 832,195,256,192 };
    AtlasRectPx thumbLevelRect{ 1105,243,256,144 };

    AtlasRectPx buttonUp{ 1388, 179, 345, 78 };
    AtlasRectPx buttonDown{ 1388, 255, 345, 78 };

    AtlasRectPx pauseIcon{ 194, 195, 26, 34};
    AtlasRectPx playIcon{ 230, 194, 29, 34};
    AtlasRectPx stepIcon{ 266, 198, 35, 26};

private:

    unsigned int prog = 0, vao = 0, vbo = 0;
    int loc_uView = -1;

    std::vector<Vertex> verts;
    int vw = 0, vh = 0;

    double mx = 0.0, my = 0.0; bool md = false, mdPrev = false;
    bool mouseConsumed = false;
    bool noRender = false;

    FontTTF font;
    bool fontReady = false;

    

    Texture2D cursorTex;
    bool curorTexReady = false;

};

static inline uint32 RGBAu32(unsigned r, unsigned g, unsigned b, unsigned a = 255) {
    return (r & 255u) | ((g & 255u) << 8) | ((b & 255u) << 16) | ((a & 255u) << 24);
}

static inline uint32 MulRGBA(uint32_t c, float m) {
    unsigned r = (unsigned)((c) & 255u);
    unsigned g = (unsigned)((c >> 8) & 255u);
    unsigned b = (unsigned)((c >> 16) & 255u);
    unsigned a = (unsigned)((c >> 24) & 255u);
    auto sat = [](int v) { return (unsigned)(v < 0 ? 0 : v > 255 ? 255 : v); };
    r = sat(int(r * m)); g = sat(int(g * m)); b = sat(int(b * m));
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

// materialAtlas.png: 10 iconos (coords en píxeles dentro de la textura)
// Orden: 0..9 (Material::Empty..Material::NpcCell)
// Ajusta estos valores a mano según tu atlas.
static constexpr AtlasRectPx kMatAtlasPx[10] = {
    {   0,      0,  32, 32 }, // 0
    {   32,     0,  32, 32 }, // 1
    {   64,     0,  32, 32 }, // 2
    {   96,     0,  32, 32 }, // 3
    {   128,    0,  32, 32 }, // 4
    {   160,    0,  32, 32 }, // 5
    {   192,    0,  32, 32 }, // 6
    {   224,    0,  32, 32 }, // 7
    {   256,    0,  32, 32 }, // 8
    {   288,    0,  32, 32 }, // 9
};
