#define STB_TRUETYPE_IMPLEMENTATION
#include "font.h"

#include <fstream>
#include <vector>

#include <glad/gl.h>

static bool ReadFileBytes(const char* path, std::vector<unsigned char>& out)
{
    out.clear();
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamoff sz = f.tellg();
    if (sz <= 0) return false;
    f.seekg(0, std::ios::beg);
    out.resize((size_t)sz);
    f.read((char*)out.data(), sz);
    return f.good();
}

bool FontTTF::Load(const char* ttfPath, float pixelHeight_, int atlasW_, int atlasH_, bool linearFilter)
{
    Destroy();

    if (!ttfPath || !*ttfPath) return false;
    if (!ReadFileBytes(ttfPath, ttfBytes)) return false;

    stbtt_fontinfo info{};
    const int offset = stbtt_GetFontOffsetForIndex(ttfBytes.data(), 0);
    if (!stbtt_InitFont(&info, ttfBytes.data(), offset)) {
        ttfBytes.clear();
        return false;
    }

    pixelHeight = pixelHeight_;
    atlasW = atlasW_;
    atlasH = atlasH_;

    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);

    ascentPx = ascent * scale;
    lineHeightPx = (ascent - descent + lineGap) * scale;

    std::vector<unsigned char> alpha;
    alpha.resize((size_t)atlasW * (size_t)atlasH);

    const int bakedRes = stbtt_BakeFontBitmap(
        ttfBytes.data(),
        0,
        pixelHeight,
        alpha.data(),
        atlasW,
        atlasH,
        FirstChar,
        CharCount,
        baked
    );

    if (bakedRes <= 0) {
        ttfBytes.clear();
        return false;
    }

    std::vector<unsigned char> rgba;
    rgba.resize((size_t)atlasW * (size_t)atlasH * 4);
    for (int y = 0; y < atlasH; ++y) {
        for (int x = 0; x < atlasW; ++x) {
            const unsigned char a = alpha[(size_t)y * (size_t)atlasW + (size_t)x];
            const size_t i = ((size_t)y * (size_t)atlasW + (size_t)x) * 4;
            rgba[i + 0] = 255;
            rgba[i + 1] = 255;
            rgba[i + 2] = 255;
            rgba[i + 3] = a;
        }
    }

    if (!atlas.Create(atlasW, atlasH, rgba.data(), linearFilter)) {
        ttfBytes.clear();
        return false;
    }

    loaded = true;
    return true;
}

void FontTTF::Destroy()
{
    loaded = false;
    atlas.Destroy();
    ttfBytes.clear();
    atlasW = atlasH = 0;
    pixelHeight = 0.0f;
    ascentPx = 0.0f;
    lineHeightPx = 0.0f;
}

int FontTTF::GlyphIndex(uint32_t codepoint) const
{
    if (codepoint < (uint32_t)FirstChar || codepoint > (uint32_t)LastChar) return -1;
    return (int)(codepoint - (uint32_t)FirstChar);
}
