#include "render/renderer.h" 
#include "render/sprite.h"
#include "app/module.h"  
#include "core/engine.h"
#include "app/app.h"
#include "core/utils.h"
#include "app/screenshot.h"

#include <vector>
#include <cstring>
#include <algorithm>


Renderer::Renderer(App* app, bool start_enabled) : Module(app, start_enabled) {};
Renderer::~Renderer() = default;

bool Renderer::Awake() {

    std::string vsSrc = ReadTextFile(SHADER_DIR "/grid.vs.glsl");
    std::string fsSrc = ReadTextFile(SHADER_DIR "/grid.fs.glsl");
    progGrid = MakeProgram(vsSrc.c_str(), fsSrc.c_str());

    glGenVertexArrays(1, &vao);
    glGenTextures(1, &tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    loc_uTex = glGetUniformLocation(progGrid, "uTex");
    loc_uGrid = glGetUniformLocation(progGrid, "uGrid");
    loc_uView = glGetUniformLocation(progGrid, "uView");
    loc_uCamPos = glGetUniformLocation(progGrid, "uCamPos");
    loc_uCamSize = glGetUniformLocation(progGrid, "uCamSize");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Paleta UBO (RGBA 0..1)
    glGenBuffers(1, &paletteUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, paletteUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 4 * 256 * 2, nullptr, GL_STATIC_DRAW);

    std::vector<float> colors(4 * 256, 0.0f);
    std::vector<float> extra(4 * 256, 0.0f);
    for (int i = 0; i < 256; ++i) {
        const MatProps& mp = matProps((uint8)i);
        colors[4 * i + 0] = mp.color.r / 255.0f;
        colors[4 * i + 1] = mp.color.g / 255.0f;
        colors[4 * i + 2] = mp.color.b / 255.0f;
        colors[4 * i + 3] = mp.color.a / 255.0f;
        extra[4 * i + 0] = mp.color.i;
    }
    glBufferSubData(GL_UNIFORM_BUFFER, 0, colors.size() * sizeof(float), colors.data());
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(float) * 4 * 256, extra.size() * sizeof(float), extra.data());

    GLuint blk = glGetUniformBlockIndex(progGrid, "Palette");
    glUniformBlockBinding(progGrid, blk, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, paletteUBO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Sprites
    sprites.Init();

    // --- Post programs ---
    std::string vsPost = vsSrc;
    std::string fsThresh = ReadTextFile(SHADER_DIR "/post_threshold.fs.glsl");
    std::string fsBlur = ReadTextFile(SHADER_DIR "/post_blur.fs.glsl");
    std::string fsComp = ReadTextFile(SHADER_DIR "/post_composite.fs.glsl");
    std::string fsFadeOverlay = ReadTextFile(SHADER_DIR "/fade_overlay.fs.glsl");

    progThresh = MakeProgram(vsPost.c_str(), fsThresh.c_str());
    progBlur = MakeProgram(vsPost.c_str(), fsBlur.c_str());
    progComposite = MakeProgram(vsPost.c_str(), fsComp.c_str());
    progFadeOverlay = MakeProgram(vsPost.c_str(), fsFadeOverlay.c_str());

    loc_th_uScene = glGetUniformLocation(progThresh, "uScene");
    loc_th_uThreshold = glGetUniformLocation(progThresh, "uThreshold");

    loc_bl_uTex = glGetUniformLocation(progBlur, "uTex");
    loc_bl_uTexel = glGetUniformLocation(progBlur, "uTexel");
    loc_bl_uHorizontal = glGetUniformLocation(progBlur, "uHorizontal");

    loc_cp_uScene = glGetUniformLocation(progComposite, "uScene");
    loc_cp_uBloom = glGetUniformLocation(progComposite, "uBloom");
    loc_cp_uExposure = glGetUniformLocation(progComposite, "uExposure");
    loc_cp_uBloomStrength = glGetUniformLocation(progComposite, "uBloomStrength");

    loc_cp_uFade = glGetUniformLocation(progComposite, "uFade");
    loc_cp_uFadeEdge = glGetUniformLocation(progComposite, "uFadeEdge");

    loc_fo_uFade = glGetUniformLocation(progFadeOverlay, "uFade");
    loc_fo_uEdge = glGetUniformLocation(progFadeOverlay, "uEdge");
    loc_fo_uView = glGetUniformLocation(progFadeOverlay, "uView");
    loc_fo_uColor = glGetUniformLocation(progFadeOverlay, "uColor");
    loc_fo_uCellPx = glGetUniformLocation(progFadeOverlay, "uCellPx");

    return true;

}
bool Renderer::Start() { return true; }
bool Renderer::PreUpdate() { return true; }

bool Renderer::Update(float dt) {

    
    int rx = 0, ry = 0, rw = 0, rh = 0;
    bool uploadedDirty = false;
    ensureSceneTargets(app->framebufferSize.x, app->framebufferSize.y);

    while (app->engine->PopChunkDirtyGPURect(rx, ry, rw, rh)) {
        Draw(app->engine->GetFrontPlane(), app->gridSize.x, app->gridSize.y, rx, ry, rw, rh);
        uploadedDirty = true;
    }

    if (!uploadedDirty && !texValid) {
        Draw(app->engine->GetFrontPlane(), app->gridSize.x, app->gridSize.y, 0, 0, 0, 0);
        
    }
    

    if (app->HasPendingTransparentScreenshot()) {
        const bool ok = SaveTransparentScreenshot(app->PendingTransparentScreenshotPath().c_str(),
            app->gridSize.x, app->gridSize.y, app->framebufferSize.x, app->framebufferSize.y);
        app->FinishTransparentScreenshot(ok);
    }

    DrawGrid(app->gridSize.x, app->gridSize.y, app->framebufferSize.x, app->framebufferSize.y);

    return true;
}
bool Renderer::PostUpdate() { return true; }
bool Renderer::CleanUp() {

    sprites.Shutdown();

    if (sceneTex) { glDeleteTextures(1, &sceneTex); sceneTex = 0; }
    if (pingTex[0]) { glDeleteTextures(1, &pingTex[0]); pingTex[0] = 0; }
    if (pingTex[1]) { glDeleteTextures(1, &pingTex[1]); pingTex[1] = 0; }
    if (sceneFBO) { glDeleteFramebuffers(1, &sceneFBO); sceneFBO = 0; }
    if (pingFBO[0]) { glDeleteFramebuffers(1, &pingFBO[0]); pingFBO[0] = 0; }
    if (pingFBO[1]) { glDeleteFramebuffers(1, &pingFBO[1]); pingFBO[1] = 0; }
    fboW = fboH = 0;

    if (paletteUBO) { glDeleteBuffers(1, &paletteUBO); paletteUBO = 0; }

    if (pbo[0] || pbo[1]) {
        glDeleteBuffers(2, pbo);
        pbo[0] = pbo[1] = 0;
    }
    pboIdx = 0;
    pboCapacity = 0;

    if (tex) { glDeleteTextures(1, &tex); tex = 0; }
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }

    if (progGrid) { glDeleteProgram(progGrid); progGrid = 0; }
    if (progThresh) { glDeleteProgram(progThresh); progThresh = 0; }
    if (progBlur) { glDeleteProgram(progBlur); progBlur = 0; }
    if (progComposite) { glDeleteProgram(progComposite); progComposite = 0; }
    if (progFadeOverlay) { glDeleteProgram(progFadeOverlay); progFadeOverlay = 0; }

    texW = texH = 0;
    texValid = false;
    scratch.clear();
    scratchRect.clear();

    return true;
}

void Renderer::Draw(const uint8* planeM, int gridW, int gridH, int x0, int y0, int rw, int rh)
{
    if (!texValid || texW != gridW || texH != gridH) {
        uploadFullCPU(planeM, gridW, gridH);
    }
    else if (rw > 0 && rh > 0) {
        scratchRect.resize(size_t(rw) * size_t(rh));
        for (int y = 0; y < rh; ++y) {
            const uint8* srcRow = &planeM[size_t(y0 + y) * size_t(gridW) + size_t(x0)];
            uint8* dstRow = &scratchRect[size_t(y) * size_t(rw)];
            std::memcpy(dstRow, srcRow, size_t(rw));
        }
        uploadRectPBO(scratchRect.data(), rw, rh, x0, y0, gridW, gridH);
    }
}

bool Renderer::SaveBackbufferScreenshot(const char* path, int viewW, int viewH)
{
    if (!path || viewW <= 0 || viewH <= 0) return false;

    std::vector<uint8> pixels((size_t)viewW * (size_t)viewH * 4u);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, viewW, viewH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return sf_screenshot::WritePngRGBA(path, pixels, viewW, viewH);
}

bool Renderer::SaveTransparentScreenshot(const char* path, int gridW, int gridH, int viewW, int viewH)
{
    if (!path || gridW <= 0 || gridH <= 0 || viewW <= 0 || viewH <= 0) return false;

    uint capSceneFBO = 0, capSceneTex = 0;
    uint capPingFBO[2] = { 0,0 }, capPingTex[2] = { 0,0 };
    uint capOutFBO = 0, capOutTex = 0;
    bool ok = false;

    auto makeTex16F = [&](uint& id) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, viewW, viewH, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };

    auto makeTex8 = [&](uint& id) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, viewW, viewH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };

    glGenFramebuffers(1, &capSceneFBO);
    makeTex16F(capSceneTex);
    glBindFramebuffer(GL_FRAMEBUFFER, capSceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, capSceneTex, 0);

    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &capPingFBO[i]);
        makeTex16F(capPingTex[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, capPingFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, capPingTex[i], 0);
    }

    glGenFramebuffers(1, &capOutFBO);
    makeTex8(capOutTex);
    glBindFramebuffer(GL_FRAMEBUFFER, capOutFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, capOutTex, 0);

    const bool outOk = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, capSceneFBO);
    const bool sceneOk = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, capPingFBO[0]);
    const bool ping0Ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, capPingFBO[1]);
    const bool ping1Ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    if (outOk && sceneOk && ping0Ok && ping1Ok) {
        DrawSceneTarget(capSceneFBO, capSceneTex, gridW, gridH, viewW, viewH, true, false, true);
        uint bloomTex = BlurSceneBloom(capSceneTex, capPingFBO[0], capPingTex[0], capPingFBO[1], capPingTex[1], viewW, viewH);
        CompositeScene(capOutFBO, capSceneTex, bloomTex, viewW, viewH, 0.0f);

        std::vector<uint8> pixels((size_t)viewW * (size_t)viewH * 4u);
        std::vector<float> scenePixels((size_t)viewW * (size_t)viewH * 4u);

        glBindFramebuffer(GL_FRAMEBUFFER, capOutFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, viewW, viewH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        glBindFramebuffer(GL_FRAMEBUFFER, capSceneFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, viewW, viewH, GL_RGBA, GL_FLOAT, scenePixels.data());

        const size_t count = (size_t)viewW * (size_t)viewH;
        for (size_t i = 0; i < count; ++i) {
            const size_t j = i * 4u;
            const float sceneA = std::clamp(scenePixels[j + 3], 0.0f, 1.0f);
            const uint8 sceneAlpha = (uint8)std::clamp(int(sceneA * 255.0f + 0.5f), 0, 255);
            const uint8 bloomAlpha = std::fmax(pixels[j + 0], std::fmax(pixels[j + 1], pixels[j + 2]));
            pixels[j + 3] = std::fmax(sceneAlpha, bloomAlpha);
        }

        ok = sf_screenshot::WritePngRGBA(path, pixels, viewW, viewH);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewW, viewH);
    glEnable(GL_BLEND);

    if (capOutTex) glDeleteTextures(1, &capOutTex);
    if (capOutFBO) glDeleteFramebuffers(1, &capOutFBO);
    for (int i = 0; i < 2; ++i) {
        if (capPingTex[i]) glDeleteTextures(1, &capPingTex[i]);
        if (capPingFBO[i]) glDeleteFramebuffers(1, &capPingFBO[i]);
    }
    if (capSceneTex) glDeleteTextures(1, &capSceneTex);
    if (capSceneFBO) glDeleteFramebuffers(1, &capSceneFBO);

    return ok;
}

void Renderer::Queue(const Sprite& s)
{
    sprites.Push(s);
}

void Renderer::DrawGrid(int w, int h, int viewW, int viewH)
{
    ensureSceneTargets(viewW, viewH);
    DrawSceneTarget(sceneFBO, sceneTex, w, h, viewW, viewH, false, true, false);
    uint bloomTex = BlurSceneBloom(sceneTex, pingFBO[0], pingTex[0], pingFBO[1], pingTex[1], viewW, viewH);
    CompositeScene(0, sceneTex, bloomTex, viewW, viewH, 0.0f);
    glEnable(GL_BLEND);
}

void Renderer::DrawSceneTarget(uint targetFBO, uint targetTex, int gridW, int gridH, int viewW, int viewH,
    bool transparentBg, bool drawBg, bool keepWorldQueue)
{
    //Color de fuera
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, viewW, viewH);

    if (transparentBg) {
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else {
        const float outR = 0.02f, outG = 0.02f, outB = 0.03f;
        const float inR = 0.06f, inG = 0.06f, inB = 0.08f;

        const float cw = std::fmax(1.0f, app->camera.size.x);
        const float ch = std::fmax(1.0f, app->camera.size.y);

        int sxCell = (int)((float)viewW / cw); if (sxCell < 1) sxCell = 1;
        int syCell = (int)((float)viewH / ch); if (syCell < 1) syCell = 1;
        int s = (sxCell < syCell) ? sxCell : syCell; if (s < 1) s = 1;

        int sizeW = (int)(cw * (float)s);
        int sizeH = (int)(ch * (float)s);
        if (sizeW > viewW) sizeW = viewW;
        if (sizeH > viewH) sizeH = viewH;

        int offX = (viewW - sizeW) / 2;
        int offY = (viewH - sizeH) / 2;
        if (offX < 0) offX = 0;
        if (offY < 0) offY = 0;

        glClearColor(outR, outG, outB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //Color Dentro
        glEnable(GL_SCISSOR_TEST);
        glScissor(offX, offY, sizeW, sizeH);
        glClearColor(inR, inG, inB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
    }

    if (drawBg) {
        sprites.Begin(viewW, viewH);
        sprites.Flush(RenderLayer::BG);
    }

    glUseProgram(progGrid);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(loc_uTex, 0);
    glUniform2f(loc_uGrid, (float)gridW, (float)gridH);
    glUniform2f(loc_uView, (float)viewW, (float)viewH);
    glUniform2f(loc_uCamPos, app->camera.pos.x, app->camera.pos.y);
    glUniform2f(loc_uCamSize, app->camera.size.x, app->camera.size.y);
    drawFullscreen();

    glEnable(GL_BLEND);
    sprites.Begin(viewW, viewH);
    sprites.Flush(RenderLayer::WORLD, !keepWorldQueue);
}

uint Renderer::BlurSceneBloom(uint sceneTexId, uint pingFbo0, uint pingTex0, uint pingFbo1, uint pingTex1,
    int viewW, int viewH)
{
    glDisable(GL_BLEND);

    glUseProgram(progThresh);
    glUniform1i(loc_th_uScene, 0);
    glUniform1f(loc_th_uThreshold, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexId);
    glBindFramebuffer(GL_FRAMEBUFFER, pingFbo0);
    glViewport(0, 0, viewW, viewH);
    drawFullscreen();

    bool horizontal = true;
    int passes = 6;
    for (int i = 0; i < passes; ++i) {
        glUseProgram(progBlur);
        glUniform1i(loc_bl_uTex, 0);
        glUniform2f(loc_bl_uTexel, 1.0f / float(viewW), 1.0f / float(viewH));
        glUniform1i(loc_bl_uHorizontal, horizontal ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, horizontal ? pingTex0 : pingTex1);
        glBindFramebuffer(GL_FRAMEBUFFER, horizontal ? pingFbo1 : pingFbo0);
        drawFullscreen();
        horizontal = !horizontal;
    }

    return horizontal ? pingTex0 : pingTex1;
}

void Renderer::CompositeScene(uint targetFBO, uint sceneTexId, uint bloomTexId, int viewW, int viewH, float fade)
{
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, viewW, viewH);

    if (targetFBO != 0) {
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glUseProgram(progComposite);
    glUniform1i(loc_cp_uScene, 0);
    glUniform1i(loc_cp_uBloom, 1);
    glUniform1f(loc_cp_uExposure, 1.0f);
    glUniform1f(loc_cp_uBloomStrength, 0.7f);

    if (loc_cp_uFade >= 0) glUniform1f(loc_cp_uFade, fade);
    if (loc_cp_uFadeEdge >= 0) glUniform1f(loc_cp_uFadeEdge, 0.08f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomTexId);
    drawFullscreen();
}

void Renderer::FlushUI(int viewW, int viewH)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewW, viewH);

    sprites.Begin(viewW, viewH);
    sprites.Flush(RenderLayer::UI);

    DrawFadeOverlay(viewW, viewH);
}

void Renderer::DrawFadeOverlay(int viewW, int viewH)
{
    const float f = app->screenFade;
    if (f <= 0.001f) return;

    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewW, viewH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(progFadeOverlay);
    if (loc_fo_uFade >= 0) glUniform1f(loc_fo_uFade, f);
    if (loc_fo_uEdge >= 0) glUniform1f(loc_fo_uEdge, 0.04f);
    if (loc_fo_uView >= 0) glUniform2f(loc_fo_uView, (float)viewW, (float)viewH);
    glUniform3f(loc_fo_uColor, 0.1f, 0.1f, 0.11f); //Color del fade to black
    if (loc_fo_uCellPx >= 0) glUniform1f(loc_fo_uCellPx, app->pixelsPerCell);

    drawFullscreen();
}

void Renderer::uploadFullCPU(const uint8* img, int w, int h) {
    if (!img || w <= 0 || h <= 0) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    if (w != texW || h != texH) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        texW = w; texH = h;
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_UNSIGNED_BYTE, img);
    texValid = true;
}

void Renderer::uploadRectPBO(const uint8* src, int rw, int rh, int x0, int y0, int texWNeeded, int texHNeeded) {
    if (rw <= 0 || rh <= 0) return;

    glBindTexture(GL_TEXTURE_2D, tex);
    if (texW != texWNeeded || texH != texHNeeded) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, texWNeeded, texHNeeded, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        texW = texWNeeded; texH = texHNeeded;
    }

    const size_t bytes = size_t(rw) * size_t(rh);
    if (pbo[0] == 0 && pbo[1] == 0) glGenBuffers(2, pbo);
    if (pboCapacity < bytes) {
        for (int i = 0;i < 2;++i) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[i]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes, nullptr, GL_STREAM_DRAW);
        }
        pboCapacity = bytes;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[pboIdx]);
    void* ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, bytes,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    std::memcpy(ptr, src, bytes);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, rw);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, rw, rh, GL_RED_INTEGER, GL_UNSIGNED_BYTE, (const void*)0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    pboIdx ^= 1;
}

void Renderer::ensureSceneTargets(int viewW, int viewH) {
    if (viewW <= 0 || viewH <= 0) return;
    if (fboW == viewW && fboH == viewH && sceneFBO) return;



    if (sceneTex) { glDeleteTextures(1, &sceneTex); sceneTex = 0; }
    if (pingTex[0]) { glDeleteTextures(1, &pingTex[0]); pingTex[0] = 0; }
    if (pingTex[1]) { glDeleteTextures(1, &pingTex[1]); pingTex[1] = 0; }
    if (sceneFBO) { glDeleteFramebuffers(1, &sceneFBO); sceneFBO = 0; }
    if (pingFBO[0]) { glDeleteFramebuffers(1, &pingFBO[0]); pingFBO[0] = 0; }
    if (pingFBO[1]) { glDeleteFramebuffers(1, &pingFBO[1]); pingFBO[1] = 0; }

    fboW = viewW; fboH = viewH;

    auto makeColorTex = [&](unsigned int& t) {
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, fboW, fboH, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        };

    glGenFramebuffers(1, &sceneFBO);
    makeColorTex(sceneTex);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);

    for (int i = 0;i < 2;++i) {
        glGenFramebuffers(1, &pingFBO[i]);
        makeColorTex(pingTex[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, pingFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingTex[i], 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::drawFullscreen()
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}





