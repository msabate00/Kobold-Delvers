#pragma once
#include <render/texture.h>
#include "../../third_party/stb/stb_truetype.h"

#include <vector>

class FontTTF {
public:
    static constexpr int FirstChar = 32;
    static constexpr int LastChar  = 255;
    static constexpr int CharCount = (LastChar - FirstChar + 1);

    bool Load(const char* ttfPath, float pixelHeight, int atlasW = 1024, int atlasH = 1024, bool linearFilter = true);
    void Destroy();

    bool IsLoaded() const { return loaded; }

    const Texture2D& Atlas() const { return atlas; }
    const stbtt_bakedchar* Baked() const { return baked; }
    int AtlasW() const { return atlasW; }
    int AtlasH() const { return atlasH; }

    float AscentPx() const { return ascentPx; }
    float LineHeightPx() const { return lineHeightPx; }

    // Map a codepoint to baked index. Returns -1 if not supported.
    int GlyphIndex(uint32_t codepoint) const;

private:
    bool loaded = false;
    Texture2D atlas;
    int atlasW = 0;
    int atlasH = 0;
    float pixelHeight = 0.0f;
    float ascentPx = 0.0f;
    float lineHeightPx = 0.0f;

    stbtt_bakedchar baked[CharCount]{};

    std::vector<unsigned char> ttfBytes;
};
