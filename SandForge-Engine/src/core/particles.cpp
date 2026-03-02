#include "particles.h"
#include "app/app.h"
#include "engine.h"
#include "render/renderer.h"
#include <algorithm>
#include <cmath>
#include <ui/ui.h>

static inline float DegToRad(float d) { return d * 0.01745329251994329577f; }
static inline float LerpF(float a, float b, float t) { return a + (b - a) * t; }

Particles::Particles(App* app, bool start_enabled) : Module(app, start_enabled) {}
Particles::~Particles() = default;

bool Particles::Awake() { return true; }

bool Particles::Start()
{
    SetMaxParticles(maxCount);
    EnsureWhiteTex();
    return true;
}

bool Particles::PreUpdate() { return true; }
bool Particles::PostUpdate() { return true; }

bool Particles::CleanUp()
{
    particles.clear();
    emitters.clear();
    freeEmitters.clear();

    if (whiteTex.id) whiteTex.Destroy();
    whiteReady = false;

    aliveCount = 0;
    spawnSerial = 1;

    return true;
}

void Particles::SetMaxParticles(int maxParticles)
{
    maxCount = (int)std::fmax(256.0f, (float)maxParticles);
    particles.reserve((size_t)maxCount);
}

// ------------------- RNG -------------------

uint32 Particles::NextU32(uint32& rng)
{
    // xorshift32
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

float Particles::Rand01(uint32& rng)
{
    const uint32 v = NextU32(rng);
    return (float)(v & 0x00FFFFFFu) / (float)0x01000000u;
}

float Particles::RangeF::Sample(uint32& rng) const
{
    if (min == max) return min;
    const float t = Particles::Rand01(rng);
    return min + (max - min) * t;
}

// ------------------- Gradients -------------------

void Particles::NormalizeGrad(GradientF& g)
{
    for (auto& k : g.keys) k.t = std::clamp(k.t, 0.0f, 1.0f);
    std::sort(g.keys.begin(), g.keys.end(), [](const GradientFKey& a, const GradientFKey& b) { return a.t < b.t; });
}

void Particles::NormalizeGrad(GradientRGBA& g)
{
    for (auto& k : g.keys) k.t = std::clamp(k.t, 0.0f, 1.0f);
    std::sort(g.keys.begin(), g.keys.end(), [](const GradientRGBAKey& a, const GradientRGBAKey& b) { return a.t < b.t; });
}

void Particles::NormalizeDesc(EmitterDesc& d)
{
    d.drag = std::fmax(0.0f, d.drag);

    if (d.lifetime.min < 0.001f) d.lifetime.min = 0.001f;
    if (d.lifetime.max < 0.001f) d.lifetime.max = 0.001f;

    NormalizeGrad(d.speed);
    NormalizeGrad(d.size);
    NormalizeGrad(d.color);

    if (d.speed.keys.empty()) {
        d.speed.keys = { { {0.0f, 0.0f}, 0.0f } };
    }
    if (d.size.keys.empty()) {
        d.size.keys = {
            { {1.0f, 1.0f}, 0.0f },
            { {0.0f, 0.0f}, 1.0f },
        };
    }
    if (d.color.keys.empty()) {
        d.color.keys = {
            { {RGBAu32(255, 255, 255, 255), RGBAu32(255, 255, 255, 255)}, 0.0f },
            { {RGBAu32(255, 255, 255, 0),   RGBAu32(255, 255, 255, 0)},   1.0f },
        };
    }
    
    //Por si aca
    if (d.maxAliveFromThisEmitter < 0) d.maxAliveFromThisEmitter = 0;
}

uint32 Particles::LerpRGBA(uint32 a, uint32 b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);

    auto lerp8 = [&](int aa, int bb) -> uint8 {
        const float v = aa + (bb - aa) * t;
        return (uint8)std::clamp((int)std::lround(v), 0, 255);
    };

    const int ar = (int)(a & 255u);
    const int ag = (int)((a >> 8) & 255u);
    const int ab = (int)((a >> 16) & 255u);
    const int aa = (int)((a >> 24) & 255u);

    const int br = (int)(b & 255u);
    const int bg = (int)((b >> 8) & 255u);
    const int bb = (int)((b >> 16) & 255u);
    const int ba = (int)((b >> 24) & 255u);

    const uint32 r = (uint32)lerp8(ar, br);
    const uint32 g = (uint32)lerp8(ag, bg);
    const uint32 bl = (uint32)lerp8(ab, bb);
    const uint32 al = (uint32)lerp8(aa, ba);

    return r | (g << 8) | (bl << 16) | (al << 24);
}

float Particles::SampleGradF(const GradientF& g, float t, float u)
{
    if (g.keys.empty()) return 0.0f;

    t = std::clamp(t, 0.0f, 1.0f);
    u = std::clamp(u, 0.0f, 1.0f);

    auto sampleKey = [&](const GradientFKey& k) -> float {
        return k.range.min + (k.range.max - k.range.min) * u;
    };

    if (g.keys.size() == 1) return sampleKey(g.keys[0]);
    if (t <= g.keys.front().t) return sampleKey(g.keys.front());
    if (t >= g.keys.back().t) return sampleKey(g.keys.back());

    for (size_t i = 0; i + 1 < g.keys.size(); ++i) {
        const auto& a = g.keys[i];
        const auto& b = g.keys[i + 1];
        if (t <= b.t) {
            const float dt = b.t - a.t;
            const float tt = (dt > 0.00001f) ? ((t - a.t) / dt) : 0.0f;
            const float mn = LerpF(a.range.min, b.range.min, tt);
            const float mx = LerpF(a.range.max, b.range.max, tt);
            return mn + (mx - mn) * u;
        }
    }

    return sampleKey(g.keys.back());
}

uint32 Particles::SampleGradC(const GradientRGBA& g, float t, float u)
{
    if (g.keys.empty()) return 0xFFFFFFFFu;

    t = std::clamp(t, 0.0f, 1.0f);
    u = std::clamp(u, 0.0f, 1.0f);

    auto sampleKey = [&](const GradientRGBAKey& k) -> uint32 {
        return LerpRGBA(k.range.min, k.range.max, u);
    };

    if (g.keys.size() == 1) return sampleKey(g.keys[0]);
    if (t <= g.keys.front().t) return sampleKey(g.keys.front());
    if (t >= g.keys.back().t) return sampleKey(g.keys.back());

    for (size_t i = 0; i + 1 < g.keys.size(); ++i) {
        const auto& a = g.keys[i];
        const auto& b = g.keys[i + 1];
        if (t <= b.t) {
            const float dt = b.t - a.t;
            const float tt = (dt > 0.00001f) ? ((t - a.t) / dt) : 0.0f;
            const uint32 mn = LerpRGBA(a.range.min, b.range.min, tt);
            const uint32 mx = LerpRGBA(a.range.max, b.range.max, tt);
            return LerpRGBA(mn, mx, u);
        }
    }

    return sampleKey(g.keys.back());
}

// ------------------- Emitters -------------------

Particles::Emitter* Particles::GetEmitter(EmitterHandle h)
{
    if (!h.valid() || h.index >= emitters.size()) return nullptr;
    Emitter& e = emitters[h.index];
    if (!e.active) return nullptr;
    if (e.generation != h.generation) return nullptr;
    if (e.destroyed) return nullptr;
    return &e;
}

uint16 Particles::AllocEmitterSlot()
{
    uint16 idx = 0xFFFF;

    if (!freeEmitters.empty()) {
        idx = freeEmitters.back();
        freeEmitters.pop_back();
    } else {
        if (emitters.size() >= 0xFFFFu) return 0xFFFF;
        idx = (uint16)emitters.size();
        emitters.emplace_back();
    }

    Emitter& e = emitters[idx];
    const uint16 gen = (e.generation == 0) ? 1 : e.generation;

    e = Emitter{};
    e.active = true;
    e.generation = gen;

    return idx;
}

void Particles::FreeEmitterSlot(uint16 idx)
{
    if (idx >= emitters.size()) return;

    Emitter& e = emitters[idx];
    e.active = false;
    e.enabled = true;
    e.destroyed = false;
    e.pendingBurst = false;
    e.spawnAcc = 0.0f;
    e.aliveFromEmitter = 0;
    e.desc = EmitterDesc{};

    //Fix para reusar los borrados pendientes (yo me entiendo)
    e.generation = (uint16)(e.generation + 1u);
    if (e.generation == 0) e.generation = 1;

    freeEmitters.push_back(idx);
}

uint32 Particles::SeedFromPos(const EmitterDesc& d, float x, float y, uint32 salt) const
{
    const float scale = d.screenSpace ? 1.0f : 256.0f;
    const int ix = (int)std::floor(x * scale);
    const int iy = (int)std::floor(y * scale);

    const int parity = app ? app->frames : 0;
    const uint32 s = salt ^ d.seedSalt;

    return (uint32)app->engine->randu32(ix, iy, parity, s);
}

Particles::EmitterHandle Particles::CreateEmitter(const EmitterDesc& desc, float x, float y)
{
    uint16 idx = AllocEmitterSlot();
    if (idx == 0xFFFF) return {};

    Emitter& e = emitters[idx];
    e.desc = desc;
    NormalizeDesc(e.desc);

    e.pos = { x, y };
    e.enabled = true;
    e.destroyed = false;
    e.pendingBurst = (e.desc.burstCount > 0);
    e.spawnAcc = 0.0f;
    e.rng = SeedFromPos(e.desc, x, y, (uint32)(idx ^ spawnSerial++));
    e.aliveFromEmitter = 0;

    return EmitterHandle{ idx, e.generation };
}

void Particles::DestroyEmitter(EmitterHandle h)
{
    Emitter* e = GetEmitter(h);
    if (!e) return;

    e->enabled = false;
    e->pendingBurst = false;
    e->desc.spawnRate = 0.0f;
    e->desc.burstCount = 0;
    e->destroyed = true;

    if (e->aliveFromEmitter == 0) {
        FreeEmitterSlot(h.index);
    }
}

void Particles::SetEmitterEnabled(EmitterHandle h, bool enabled)
{
    Emitter* e = GetEmitter(h);
    if (!e) return;
    e->enabled = enabled;
}

void Particles::SetEmitterPos(EmitterHandle h, float x, float y)
{
    Emitter* e = GetEmitter(h);
    if (!e) return;
    e->pos = { x, y };
}

void Particles::SetEmitterDesc(EmitterHandle h, const EmitterDesc& desc)
{
    Emitter* e = GetEmitter(h);
    if (!e) return;

    e->desc = desc;
    NormalizeDesc(e->desc);
    e->pendingBurst = (e->desc.burstCount > 0);
}

void Particles::EmitBurst(const EmitterDesc& desc, float x, float y, int countOverride)
{
    const int n = (countOverride >= 0) ? countOverride : desc.burstCount;
    if (n <= 0) return;

    EnsureWhiteTex();

    uint16 idx = AllocEmitterSlot();
    if (idx == 0xFFFF) return;

    Emitter& e = emitters[idx];
    e.desc = desc;
    e.desc.spawnRate = 0.0f;
    e.desc.burstCount = 0;
    NormalizeDesc(e.desc);

    e.pos = { x, y };
    e.enabled = false;
    e.destroyed = true;
    e.pendingBurst = false;
    e.spawnAcc = 0.0f;
    e.rng = SeedFromPos(e.desc, x, y, (uint32)(idx ^ spawnSerial++));
    e.aliveFromEmitter = 0;

    for (int i = 0; i < n; ++i) {
        if (e.desc.maxAliveFromThisEmitter > 0 && e.aliveFromEmitter >= e.desc.maxAliveFromThisEmitter) break;
        SpawnParticle(idx, e, e.desc, x, y);
    }

    if (e.aliveFromEmitter == 0) {
        FreeEmitterSlot(idx);
    }
}

// ------------------- Particles -------------------

void Particles::EnsureWhiteTex()
{
    if (whiteReady) return;

    const unsigned char px[4] = { 255,255,255,255 };
    whiteTex.Create(1, 1, px, true);
    whiteReady = true;
}

void Particles::SpawnParticle(uint16 emitterIndex, Emitter& e, const EmitterDesc& d, float x, float y)
{
    if (aliveCount >= maxCount) return;

    uint32& rng = e.rng;

    const float ang = DegToRad(d.angleDeg.Sample(rng));
    const float uSize = Rand01(rng);
    const float uSpeed = Rand01(rng);
    const float uColor = Rand01(rng);

    const float baseSpeed = std::fmax(0.0f, SampleGradF(d.speed, 0.0f, uSpeed));
    Vec2<float> vel{ std::cos(ang) * baseSpeed, std::sin(ang) * baseSpeed };

    Particle p{};
    p.pos = { x, y };
    p.vel = vel;
    p.age = 0.0f;
    p.life = std::fmax(0.001f, d.lifetime.Sample(rng));

    p.uSize = uSize;
    p.uSpeed = uSpeed;
    p.uColor = uColor;
    p.baseSpeed = baseSpeed;

    p.gravityY = d.gravityY;
    p.drag = d.drag;

    p.layer = d.layer;
    p.sort = d.sort;
    p.screenSpace = d.screenSpace;

    p.tex = d.texture ? d.texture : &whiteTex;
    p.uv = d.uv;

    p.emitterIndex = emitterIndex;

    particles.push_back(p);
    aliveCount++;
    e.aliveFromEmitter++;
}

void Particles::KillParticle(size_t i)
{
    if (i >= particles.size()) return;

    Particle& p = particles[i];
    if (p.emitterIndex != 0xFFFF && p.emitterIndex < emitters.size()) {
        Emitter& e = emitters[p.emitterIndex];
        if (e.active) {
            e.aliveFromEmitter = std::fmax(0, e.aliveFromEmitter - 1);
        }
    }

    particles[i] = particles.back();
    particles.pop_back();

    aliveCount = std::fmax(0, aliveCount - 1);
}

bool Particles::Update(float dt)
{
    if (!app || !app->renderer) return true;
    if (dt < 0.0f) dt = 0.0f;

    EnsureWhiteTex();

    //Spawn emitters
    for (uint16 i = 0; i < (uint16)emitters.size(); ++i) {
        Emitter& e = emitters[i];
        if (!e.active) continue;
        if (e.destroyed) continue;
        if (!e.enabled) continue;

        if (e.pendingBurst) {
            e.pendingBurst = false;
            const int n = std::fmax(0, e.desc.burstCount);
            for (int k = 0; k < n; ++k) {
                if (e.desc.maxAliveFromThisEmitter > 0 && e.aliveFromEmitter >= e.desc.maxAliveFromThisEmitter) break;
                SpawnParticle(i, e, e.desc, e.pos.x, e.pos.y);
            }
        }

        if (e.desc.spawnRate > 0.0f) {
            e.spawnAcc += dt * e.desc.spawnRate;
            int n = (int)std::floor(e.spawnAcc);
            if (n > 0) e.spawnAcc -= (float)n;

            for (int k = 0; k < n; ++k) {
                if (e.desc.maxAliveFromThisEmitter > 0 && e.aliveFromEmitter >= e.desc.maxAliveFromThisEmitter) break;
                SpawnParticle(i, e, e.desc, e.pos.x, e.pos.y);
            }
        }
    }

    //Valores de la camara
    const float viewW = (float)app->framebufferSize.x;
    const float viewH = (float)app->framebufferSize.y;

    const float camX = app->camera.pos.x;
    const float camY = app->camera.pos.y;
    const float camW = app->camera.size.x;
    const float camH = app->camera.size.y;

    const float sxCell = std::floor(viewW / std::fmax(1.0f, camW));
    const float syCell = std::floor(viewH / std::fmax(1.0f, camH));
    const float s = std::fmax(1.0f, std::fmin(sxCell, syCell));

    const float sizeW = camW * s;
    const float sizeH = camH * s;
    const float offX = (viewW - sizeW) * 0.5f;
    const float offY = (viewH - sizeH) * 0.5f;

    //Simulacion y renderer
    for (size_t i = 0; i < particles.size();) {
        Particle& p = particles[i];

        if (p.emitterIndex >= emitters.size() || !emitters[p.emitterIndex].active) {
            KillParticle(i);
            continue;
        }

        const EmitterDesc& d = emitters[p.emitterIndex].desc;

        p.age += dt;
        if (p.age >= p.life) {
            KillParticle(i);
            continue;
        }

        p.vel.y += p.gravityY * dt;
        if (p.drag > 0.0f) {
            const float k = std::fmax(0.0f, 1.0f - p.drag * dt);
            p.vel.x *= k;
            p.vel.y *= k;
        }

        const float t = std::clamp(p.age / p.life, 0.0f, 1.0f);

        float speedMul = 1.0f;
        if (p.baseSpeed > 0.0001f) {
            const float curSpd = std::fmax(0.0f, SampleGradF(d.speed, t, p.uSpeed));
            speedMul = curSpd / p.baseSpeed;
        }

        p.pos.x += p.vel.x * dt * speedMul;
        p.pos.y += p.vel.y * dt * speedMul;

        const float size = std::fmax(0.0f, SampleGradF(d.size, t, p.uSize));
        if (size <= 0.0001f) {
            ++i;
            continue;
        }

        const uint32 col = SampleGradC(d.color, t, p.uColor);

        //Renderer
        Sprite spr{};
        spr.tex = p.tex ? p.tex : &whiteTex;
        spr.color = col;
        spr.layer = p.layer;
        spr.sort = p.sort;

        UVRect uv = p.uv;

        if (p.screenSpace) {
            const float x = p.pos.x - size * 0.5f;
            const float y = p.pos.y - size * 0.5f;
            const float w = size;
            const float h = size;

            const float ix0 = std::fmax(0.0f, x);
            const float iy0 = std::fmax(0.0f, y);
            const float ix1 = std::fmin(viewW, x + w);
            const float iy1 = std::fmin(viewH, y + h);
            if (ix1 > ix0 && iy1 > iy0) {
                const float cu0 = (ix0 - x) / w;
                const float cv0 = (iy0 - y) / h;
                const float cu1 = (ix1 - x) / w;
                const float cv1 = (iy1 - y) / h;

                spr.x = std::floor(ix0);
                spr.y = std::floor(iy0);
                spr.w = std::floor(ix1 - ix0);
                spr.h = std::floor(iy1 - iy0);

                spr.u0 = uv.u0 + (uv.u1 - uv.u0) * cu0;
                spr.v0 = uv.v0 + (uv.v1 - uv.v0) * cv0;
                spr.u1 = uv.u0 + (uv.u1 - uv.u0) * cu1;
                spr.v1 = uv.v0 + (uv.v1 - uv.v0) * cv1;

                app->renderer->Queue(spr);
            }
        } else {
            const float wx = p.pos.x - size * 0.5f;
            const float wy = p.pos.y - size * 0.5f;

            const float sx0 = offX + (wx - camX) * s;
            const float sy0 = offY + (wy - camY) * s;
            const float sw = size * s;
            const float sh = size * s;

            const float ix0 = std::fmax(0.0f, sx0);
            const float iy0 = std::fmax(0.0f, sy0);
            const float ix1 = std::fmin(viewW, sx0 + sw);
            const float iy1 = std::fmin(viewH, sy0 + sh);
            if (ix1 > ix0 && iy1 > iy0) {
                const float cu0 = (ix0 - sx0) / sw;
                const float cv0 = (iy0 - sy0) / sh;
                const float cu1 = (ix1 - sx0) / sw;
                const float cv1 = (iy1 - sy0) / sh;

                spr.x = std::floor(ix0);
                spr.y = std::floor(iy0);
                spr.w = std::floor(ix1 - ix0);
                spr.h = std::floor(iy1 - iy0);

                spr.u0 = uv.u0 + (uv.u1 - uv.u0) * cu0;
                spr.v0 = uv.v0 + (uv.v1 - uv.v0) * cv0;
                spr.u1 = uv.u0 + (uv.u1 - uv.u0) * cu1;
                spr.v1 = uv.v0 + (uv.v1 - uv.v0) * cv1;

                app->renderer->Queue(spr);
            }
        }

        ++i;
    }

    for (uint16 i = 0; i < (uint16)emitters.size(); ++i) {
        Emitter& e = emitters[i];
        if (!e.active) continue;
        if (!e.destroyed) continue;
        if (e.aliveFromEmitter != 0) continue;
        FreeEmitterSlot(i);
    }

    return true;
}

// ------------------- Presets -------------------

Particles::EmitterDesc Particles::PresetSparks()
{
    EmitterDesc d{};
    d.spawnRate = 0.0f;
    d.burstCount = 16;
    d.lifetime = { 0.12f, 0.35f };

    d.speed = {
        {{12.0f, 40.0f}, 0.0f},
    };

    d.angleDeg = { -110.0f, -70.0f };
    d.gravityY = 60.0f;
    d.drag = 0.5f;

    d.size = {
        {{0.6f, 1.4f}, 0.0f},
        {{0.0f, 0.0f}, 1.0f},
    };

    d.color = {
        {{RGBAu32(160, 226, 255, 255), RGBAu32(160, 226, 255, 255)}, 0.0f},
        {{RGBAu32(32,  90,  255,   0), RGBAu32(32,  90,  255,   0)}, 1.0f},
    };

    d.layer = RenderLayer::WORLD;
    d.sort = 10;
    d.screenSpace = false;
    d.maxAliveFromThisEmitter = 256;
    return d;
}

Particles::EmitterDesc Particles::PresetSmoke()
{
    EmitterDesc d{};
    d.spawnRate = 0.0f;
    d.burstCount = 10;
    d.lifetime = { 0.6f, 1.4f };

    d.speed = {
        {{2.0f, 8.0f}, 0.0f},
    };

    d.angleDeg = { -95.0f, -85.0f };
    d.gravityY = -8.0f;
    d.drag = 0.2f;

    d.size = {
        {{1.0f, 2.0f}, 0.0f},
        {{2.2f, 3.8f}, 1.0f},
    };

    d.color = {
        {{RGBAu32(176, 176, 176, 160), RGBAu32(176, 176, 176, 160)}, 0.0f},
        {{RGBAu32(80,   80,  80,   0), RGBAu32(80,   80,  80,   0)}, 1.0f},
    };

    d.layer = RenderLayer::WORLD;
    d.sort = 5;
    d.screenSpace = false;
    d.maxAliveFromThisEmitter = 512;
    return d;
}

Particles::EmitterDesc Particles::PresetDust()
{
    EmitterDesc d{};
    d.spawnRate = 0.0f;
    d.burstCount = 14;
    d.lifetime = { 0.25f, 0.6f };

    d.speed = {
        {{6.0f, 18.0f}, 0.0f},
    };

    d.angleDeg = { 160.0f, 380.0f };
    d.gravityY = 35.0f;
    d.drag = 1.2f;

    d.size = {
        {{0.8f, 1.6f}, 0.0f},
        {{0.0f, 0.4f}, 1.0f},
    };

    d.color = {
        {{RGBAu32(176, 192, 200, 176), RGBAu32(176, 192, 200, 176)}, 0.0f},
        {{RGBAu32(176, 192, 200,   0), RGBAu32(176, 192, 200,   0)}, 1.0f},
    };

    d.layer = RenderLayer::WORLD;
    d.sort = 3;
    d.screenSpace = false;
    d.maxAliveFromThisEmitter = 512;
    return d;
}

Particles::EmitterDesc Particles::PresetFire()
{
    EmitterDesc d{};
    d.spawnRate = 42.0f;
    d.burstCount = 0;
    d.lifetime = { 0.22f, 0.55f };

    d.speed = {
        {{1.8f, 7.0f}, 0.0f},
    };

    d.angleDeg = { -115.0f, -65.0f };
    d.gravityY = -10.0f;
    d.drag = 0.35f;

    d.size = {
        {{0.35f, 0.95f}, 0.0f},
        {{0.05f, 0.35f}, 1.0f},
    };

    d.color = {
        {{RGBAu32(255, 170, 70, 220), RGBAu32(255, 170, 70, 220)}, 0.0f},
        {{RGBAu32(80,   20,  0,   0), RGBAu32(80,   20,  0,   0)}, 1.0f},
    };

    d.layer = RenderLayer::WORLD;
    d.sort = 6;
    d.screenSpace = false;
    d.seedSalt = 1;
    d.maxAliveFromThisEmitter = 1024;
    return d;
}

Particles::EmitterDesc Particles::PresetConfettiUI()
{
    EmitterDesc d{};
    d.spawnRate = 0.0f;
    d.burstCount = 48;
    d.lifetime = { 0.85f, 1.65f };

    d.speed = {
        {{180.0f, 460.0f}, 0.0f},
    };

    d.angleDeg = { -125.0f, -55.0f };
    d.gravityY = 900.0f;
    d.drag = 0.06f;

    d.size = {
        {{6.0f, 11.0f}, 0.0f},
        {{3.0f,  7.0f}, 1.0f},
    };

    d.color = {
        {{RGBAu32(255, 80, 200, 230), RGBAu32(255, 80, 200, 230)}, 0.0f},
        {{RGBAu32(255, 80, 200,   0), RGBAu32(255, 80, 200,   0)}, 1.0f},
    };

    d.layer = RenderLayer::UI;
    d.sort = 200;
    d.screenSpace = true;
    d.uv = { 0,0,1,1 };
    d.seedSalt = 2;
    d.maxAliveFromThisEmitter = 2048;
    return d;
}

Particles::EmitterDesc Particles::PresetShockwave()
{
    EmitterDesc d{};
    d.spawnRate = 0.0f;
    d.burstCount = 1;
    d.lifetime = { 0.12f, 0.18f };

    d.speed = {
        {{0.0f, 0.0f}, 0.0f},
    };

    d.angleDeg = { 0.0f, 0.0f };
    d.gravityY = 0.0f;
    d.drag = 0.0f;

    d.size = {
        {{0.6f, 1.0f}, 0.0f},
        {{5.0f, 8.5f}, 1.0f},
    };

    d.color = {
        {{RGBAu32(255, 255, 255, 190), RGBAu32(255, 255, 255, 190)}, 0.0f},
        {{RGBAu32(255, 255, 255,   0), RGBAu32(255, 255, 255,   0)}, 1.0f},
    };

    d.layer = RenderLayer::WORLD;
    d.sort = 50;
    d.screenSpace = false;
    d.uv = { 0,0,1,1 };

    d.seedSalt = 3;
    d.maxAliveFromThisEmitter = 64;
    return d;
}
