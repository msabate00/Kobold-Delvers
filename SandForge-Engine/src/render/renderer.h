#pragma once
#include "app/module.h"
#include "core/material.h"
#include "sprite.h"
#include <vector>
#include <cstdint>

class Renderer : public Module {
public:
    Renderer(App* app, bool start_enabled = true);
    virtual ~Renderer();

    bool Awake();
    bool Start();
    bool PreUpdate();
    bool Update(float dt);
    bool PostUpdate();

    bool CleanUp();


    void Draw(const uint8* planeM, int gridW, int gridH, int x0, int y0, int rw, int rh);
    void Queue(const Sprite& s);

    //Solo afecta: RenderLayer::UI
    void FlushUI(int viewW, int viewH);

private:

    void DrawGrid(const std::vector<uint8>& indices, int w, int h, int viewW, int viewH);

    void uploadFullCPU(const uint8* img, int w, int h);
    void uploadRectPBO(const uint8* src, int rw, int rh, int x0, int y0,
        int texWNeeded, int texHNeeded);
    void ensureSceneTargets(int viewW, int viewH);
    void drawFullscreen();

    void DrawFadeOverlay(int viewW, int viewH);

public:

private:
    uint progGrid = 0, vao = 0, tex = 0;;
    int texW = 0, texH = 0;
    bool texValid = false;

    uint pbo[2] = { 0,0 };
    int pboIdx = 0;
    size_t pboCapacity = 0; // bytes

    uint paletteUBO = 0;

    int loc_uTex = -1;
    int loc_uGrid = -1;
    int loc_uView = -1;
    int loc_uCamPos = -1;
    int loc_uCamSize = -1;

    uint progThresh = 0, progBlur = 0, progComposite = 0;
    int loc_th_uScene = -1, loc_th_uThreshold = -1;
    int loc_bl_uTex = -1, loc_bl_uTexel = -1, loc_bl_uHorizontal = -1;
    int loc_cp_uScene = -1, loc_cp_uBloom = -1, loc_cp_uExposure = -1, loc_cp_uBloomStrength = -1;
    int loc_cp_uFade = -1, loc_cp_uFadeEdge = -1;

    //Fade
    uint progFadeOverlay = 0;
    int loc_fo_uFade = -1, loc_fo_uEdge = -1, loc_fo_uView = -1;

    uint sceneFBO = 0, sceneTex = 0;
    uint pingFBO[2] = { 0,0 }, pingTex[2] = { 0,0 };
    int fboW = 0, fboH = 0;

    std::vector<uint8> scratch;     // full
    std::vector<uint8> scratchRect; // rect

    SpriteBatch sprites;
};