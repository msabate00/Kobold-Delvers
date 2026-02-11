#pragma once
#include "app/module.h"
#include "font.h"
#include <vector>
#include <cstdint>
#include <core/material.h>
#include <render/sprite.h>
#include <render/texture.h>


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
    bool ButtonAtlas(float x, float y, float w, float h,
        int atlasIndex, uint32 rgba, uint32 rgbaHover, uint32 rgbaActive);
    bool Slider(float x, float y, float w, float h,
        float minv, float maxv, float& v, uint32 track, uint32 knob);

    void Circle(float cx, float cy, float r, uint32 c, int segments = 0);
    void Ring(float cx, float cy, float r, float t, uint32 c, int segments = 0);


    void Rect(float x, float y, float w, float h, uint32 rgba);
    void RectBorders(float x, float y, float w, float h, float t, uint32 rgba);

    void Image(const Texture2D& t, float x, float y, float w, float h,
        uint32 tint = 0xFFFFFFFF);

    void Text(float x, float y, const char* text, uint32 rgba, float scale = 1.0f);
    void TextCentered(float x, float y, float w, float h, const char* text, uint32 rgba, float scale = 1.0f);


private:

    void Flush();

public:


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

    Texture2D matAtlas;
    bool matAtlasReady = false;

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


struct AtlasRectPx { int x = 0, y = 0, w = 0, h = 0; };
struct UVRect { float u0 = 0, v0 = 0, u1 = 1, v1 = 1; };

static inline UVRect UVFromPx(const Texture2D& tex, const AtlasRectPx& r)
{
    const float invW = (tex.w > 0) ? (1.0f / (float)tex.w) : 0.0f;
    const float invH = (tex.h > 0) ? (1.0f / (float)tex.h) : 0.0f;
    return {
        r.x * invW,
        r.y * invH,
        (r.x + r.w) * invW,
        (r.y + r.h) * invH
    };
}

static inline UVRect WhitePixelUV(const Texture2D& tex)
{
    const int px = (tex.w > 0) ? (tex.w - 1) : 0;
    const int py = (tex.h > 0) ? (tex.h - 1) : 0;
    const AtlasRectPx r{ px, py, 1, 1 };
    return UVFromPx(tex, r);
}

static constexpr int kMatAtlasSize = 10;
static constexpr AtlasRectPx kMatAtlasPx[kMatAtlasSize] = {
    {  1,  2, 31, 28 }, // 0
    { 32, 12, 32, 20 }, // 1
    { 64, 21, 32, 10 }, // 2
    { 97,  1, 30, 30 }, // 3
    {129,  1, 30, 30 }, // 4
    {167,  3, 18, 26 }, // 5
    {192, 21, 32, 10 }, // 6
    {226,  3, 29, 26 }, // 7
    {258,  3, 29, 26 }, // 8
    {298,  4, 13, 24 }, // 9
};
