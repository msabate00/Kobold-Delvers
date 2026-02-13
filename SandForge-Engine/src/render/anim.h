#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <utility>

#include "render/sprite.h"
#include "render/atlas.h"

enum class AnimLoopMode : uint8_t {
    None = 0,
    Loop,
    PingPong
};

struct SpriteAnimFrame {
    const Texture2D* tex = nullptr;
    UVRect uv{ 0,0,1,1 };
    float duration = 0.0f; //segundos
};

struct SpriteAnimClip {
    std::string name;
    const Texture2D* defaultTex = nullptr;
    float fps = 12.0f;
    AnimLoopMode loop = AnimLoopMode::Loop;
    std::vector<SpriteAnimFrame> frames;

    bool valid() const { return !frames.empty() && (defaultTex || frames[0].tex); }
};

static inline SpriteAnimFrame AnimFramePx(const Texture2D* tex, const AtlasRectPx& r, float duration = 0.0f)
{
    SpriteAnimFrame f;
    f.tex = tex;
    if (tex) f.uv = UVFromPx(*tex, r);
    f.duration = duration;
    return f;
}

class SpriteAnimLibrary {
public:
    const SpriteAnimClip* Find(const std::string& name) const {
        auto it = clips.find(name);
        return it == clips.end() ? nullptr : &it->second;
    }

    SpriteAnimClip& Add(const std::string& name) {
        auto& c = clips[name];
        c.name = name;
        return c;
    }

    void Clear() { clips.clear(); }

private:
    std::unordered_map<std::string, SpriteAnimClip> clips;
};

class SpriteAnimPlayer {
public:
    void SetLibrary(const SpriteAnimLibrary* lib_) { lib = lib_; }

    bool Play(const std::string& clipName, bool restart = true) {
        if (!lib) return false;
        const SpriteAnimClip* c = lib->Find(clipName);
        if (!c || !c->valid()) return false;

        if (!restart && clip == c && playing) return true;
        clip = c;
        name = clipName;
        playing = true;
        finished = false;
        timeAcc = 0.0f;
        dir = +1;
        frame = 0;
        return true;
    }

    void Stop() {
        playing = false;
        finished = false;
        timeAcc = 0.0f;
        frame = 0;
        dir = +1;
    }

    void Update(float dt) {
        if (!playing || !clip || clip->frames.empty()) return;
        if (dt <= 0.0f || speed == 0.0f) return;

        float t = dt * speed;
        if (t < 0.0f) t = -t;

        while (t > 0.0f && playing) {
            float fd = FrameDuration(frame);
            float remaining = fd - timeAcc;
            if (t < remaining) { timeAcc += t; break; }

            t -= remaining;
            timeAcc = 0.0f;
            Step();
        }
    }

    void ApplyTo(Sprite& s, bool flipX = false) const {
        if (!clip || clip->frames.empty()) return;
        const SpriteAnimFrame& f = clip->frames[ClampFrame(frame)];
        const Texture2D* t = f.tex ? f.tex : clip->defaultTex;
        if (t) s.tex = t;
        s.u0 = f.uv.u0; s.v0 = f.uv.v0;
        s.u1 = f.uv.u1; s.v1 = f.uv.v1;
        if (flipX) std::swap(s.u0, s.u1);
    }

    bool IsPlaying() const { return playing; }
    bool IsFinished() const { return finished; }
    const std::string& CurrentName() const { return name; }
    int CurrentFrame() const { return frame; }

    void SetSpeed(float s) { speed = s; }
    float Speed() const { return speed; }

private:
    int ClampFrame(int i) const {
        if (!clip || clip->frames.empty()) return 0;
        if (i < 0) return 0;
        if (i >= (int)clip->frames.size()) return (int)clip->frames.size() - 1;
        return i;
    }

    float FrameDuration(int i) const {
        if (!clip || clip->frames.empty()) return 0.0f;
        const SpriteAnimFrame& f = clip->frames[ClampFrame(i)];
        if (f.duration > 0.0f) return f.duration;
        const float fps = (clip->fps > 0.0f) ? clip->fps : 1.0f;
        return 1.0f / fps;
    }

    void Step() {
        if (!clip) return;
        const int n = (int)clip->frames.size();
        if (n <= 1) {
            if (clip->loop == AnimLoopMode::None) { playing = false; finished = true; }
            return;
        }

        if (clip->loop == AnimLoopMode::PingPong) {
            int next = frame + dir;
            if (next >= n) { dir = -1; next = n - 2; }
            else if (next < 0) { dir = +1; next = 1; }
            frame = next;
            return;
        }

        int next = frame + 1;
        if (next < n) { frame = next; return; }

        if (clip->loop == AnimLoopMode::Loop) {
            frame = 0;
        }
        else {
            frame = n - 1;
            playing = false;
            finished = true;
        }
    }

private:
    const SpriteAnimLibrary* lib = nullptr;
    const SpriteAnimClip* clip = nullptr;
    std::string name;

    int frame = 0;
    int dir = +1; // pingpong
    float timeAcc = 0.0f;
    float speed = 1.0f;
    bool playing = false;
    bool finished = false;
};
