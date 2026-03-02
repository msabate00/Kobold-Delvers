#pragma once
#include "app/module.h"
#include "app/defs.h"
#include "render/sprite.h"
#include "render/texture.h"
#include "render/atlas.h"
#include <vector>

// Example (burst):
//   auto d = Particles::PresetSparks();
//   app->particles->EmitBurst(d, (float)wx + 0.5f, (float)wy + 0.5f, 24);
//
// Example (continuous):
//   auto h = app->particles->CreateEmitter(Particles::PresetSmoke(), x, y);
//   app->particles->SetEmitterEnabled(h, true);

class Particles : public Module {
public:

    //Estructuras float
    struct RangeF {
        float min = 0.0f;
        float max = 0.0f;
        float Sample(uint32& rng) const;
    };

    struct GradientFKey {
        RangeF range{};
        float t = 0.0f;
    };

    struct GradientF {
        std::vector<GradientFKey> keys;
        GradientF() = default;
        GradientF(std::initializer_list<GradientFKey> init) : keys(init) {}
    };

    //Estructuras color
    struct RangeRGBA {
        uint32 min = RGBAu32(255, 255, 255, 255);
        uint32 max = RGBAu32(255, 255, 255, 255);
    };

    struct GradientRGBAKey {
        RangeRGBA range{};
        float t = 0.0f;
    };

    struct GradientRGBA {
        std::vector<GradientRGBAKey> keys;
        GradientRGBA() = default;
        GradientRGBA(std::initializer_list<GradientRGBAKey> init) : keys(init) {}
    };

    struct EmitterDesc {
        // Spawn
        float spawnRate = 0.0f;
        int   burstCount = 0;

        // Movimiento
        GradientF speed{};
        RangeF angleDeg{ 0.0f, 0.0f }; //Empieza por la derecha: 0 = right, 90 = down, -90 up
        float gravityY = 0.0f;
        float drag = 0.0f;

        // Visual
        RangeF lifetime{ 0.5f, 1.0f };
        GradientF size{};
        GradientRGBA color{};

        // Rendering
        RenderLayer layer = RenderLayer::WORLD;
        int sort = 0;
        bool screenSpace = false;
        const Texture2D* texture = nullptr;
        UVRect uv{ 0,0,1,1 };

        int maxAliveFromThisEmitter = 512;

        // RNG
        uint32 seedSalt = 0;
    };

    struct EmitterHandle {
        uint16 index = 0xFFFF;
        uint16 generation = 0;
        bool valid() const { return generation != 0 && index != 0xFFFF; }
        explicit operator bool() const { return valid(); }
        static constexpr EmitterHandle null() { return {}; }
    };

public:
    Particles(App* app, bool start_enabled = true);
    virtual ~Particles();

    bool Awake();
    bool Start();
    bool PreUpdate();
    bool Update(float dt);
    bool PostUpdate();
    bool CleanUp();

    void SetMaxParticles(int maxParticles);
    int  MaxParticles() const { return maxCount; }
    int  AliveParticles() const { return aliveCount; }

    // Emitters
    EmitterHandle CreateEmitter(const EmitterDesc& desc, float x, float y);
    void DestroyEmitter(EmitterHandle h);
    void SetEmitterEnabled(EmitterHandle h, bool enabled);
    void SetEmitterPos(EmitterHandle h, float x, float y);
    void SetEmitterDesc(EmitterHandle h, const EmitterDesc& desc);

    // One shot
    void EmitBurst(const EmitterDesc& desc, float x, float y, int countOverride = -1);
    void EmitOne(const EmitterDesc& desc, float x, float y) { EmitBurst(desc, x, y, 1); }

    // Presets
    static EmitterDesc PresetSparks();
    static EmitterDesc PresetSmoke();
    static EmitterDesc PresetDust();
    static EmitterDesc PresetFire();
    static EmitterDesc PresetConfettiUI();
    static EmitterDesc PresetShockwave();

private:
    struct Particle {
        Vec2<float> pos{};
        Vec2<float> vel{};
        float age = 0.0f;
        float life = 1.0f;

        float uSize = 0.5f;
        float uSpeed = 0.5f;
        float uColor = 0.5f;

        float baseSpeed = 0.0f;

        float gravityY = 0.0f;
        float drag = 0.0f;

        RenderLayer layer = RenderLayer::WORLD;
        int sort = 0;
        bool screenSpace = false;

        const Texture2D* tex = nullptr;
        UVRect uv{ 0,0,1,1 };

        uint16 emitterIndex = 0xFFFF;
    };

    struct Emitter {
        bool active = false;
        bool enabled = true;
        bool destroyed = false;

        uint16 generation = 1;
        EmitterDesc desc{};
        Vec2<float> pos{};

        bool pendingBurst = false;
        float spawnAcc = 0.0f;

        uint32 rng = 0x12345678u;
        int aliveFromEmitter = 0;
    };

private:
    Emitter* GetEmitter(EmitterHandle h);
    uint16   AllocEmitterSlot();
    void     FreeEmitterSlot(uint16 idx);

    void NormalizeDesc(EmitterDesc& d);
    static void NormalizeGrad(GradientF& g);
    static void NormalizeGrad(GradientRGBA& g);

    uint32 SeedFromPos(const EmitterDesc& d, float x, float y, uint32 salt) const;

    void SpawnParticle(uint16 emitterIndex, Emitter& e, const EmitterDesc& d, float x, float y);
    void KillParticle(size_t i);

    static uint32 NextU32(uint32& rng);
    static float  Rand01(uint32& rng);

    static uint32 LerpRGBA(uint32 a, uint32 b, float t);
    static float  SampleGradF(const GradientF& g, float t, float u);
    static uint32 SampleGradC(const GradientRGBA& g, float t, float u);

    void EnsureWhiteTex();

private:
    Texture2D whiteTex{};
    bool whiteReady = false;

    std::vector<Particle> particles;
    std::vector<Emitter> emitters;
    std::vector<uint16> freeEmitters;

    int maxCount = 8192;
    int aliveCount = 0;
    uint32 spawnSerial = 1;
};
