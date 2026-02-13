#pragma once
#include "render/texture.h"

struct AtlasRectPx { int x = 0, y = 0, w = 0, h = 0; };

struct UVRect {
    float u0 = 0, v0 = 0;
    float u1 = 1, v1 = 1;
};

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
