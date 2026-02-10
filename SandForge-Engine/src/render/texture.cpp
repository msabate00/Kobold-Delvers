#define STB_IMAGE_IMPLEMENTATION
#include "texture.h"
#include "../../third_party/stb/stb_image.h"
#include <glad/gl.h>

bool Texture2D::Load(const char* path) {
    int ch = 0;
    stbi_set_flip_vertically_on_load(0);
    unsigned char* px = stbi_load(path, &w, &h, &ch, 4);
    if (!px) return false;

    if (!id) glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);
    return true;
}

bool Texture2D::Create(int w_, int h_, const void* rgbaBytes, bool linearFilter)
{
    if (!rgbaBytes || w_ <= 0 || h_ <= 0) return false;

    w = w_;
    h = h_;

    if (!id) glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    const GLint filter = linearFilter ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, rgbaBytes);
    return true;
}

void Texture2D::Destroy() { if (id) { glDeleteTextures(1, &id); id = 0; } }
