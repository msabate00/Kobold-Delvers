#include "ui_anim.h"

float UIAnim::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

float UIAnim::EaseOutCubic(float t)
{
    t = Clamp01(t);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

float UIAnim::EaseOutBack(float t)
{
    t = Clamp01(t);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    const float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

uint32 UIAnim::AlphaRGBA(uint32 c, float aMul)
{
    aMul = Clamp01(aMul);

    const unsigned r = c & 255u;
    const unsigned g = (c >> 8) & 255u;
    const unsigned b = (c >> 16) & 255u;
    unsigned a = (c >> 24) & 255u;
    a = (unsigned)(a * aMul);

    return r | (g << 8) | (b << 16) | (a << 24);
}

float UIAnim::Approach(float v, float target, float speed, float dt)
{
    if (v < target) {
        v += speed * dt;
        if (v > target) v = target;
    }
    else if (v > target) {
        v -= speed * dt;
        if (v < target) v = target;
    }
    return v;
}

float UIAnim::Hover(float v, bool hover, float speed, float dt)
{
    return Approach(v, hover ? 1.0f : 0.0f, speed, dt);
}

float UIAnim::IntroT(float timer, float delay, float duration)
{
    if (duration <= 0.0f) return 1.0f;
    return Clamp01((timer - delay) / duration);
}

float UIAnim::IntroAlpha(float timer, float delay, float duration)
{
    return EaseOutCubic(IntroT(timer, delay, duration));
}

float UIAnim::IntroMove(float timer, float delay, float duration)
{
    return EaseOutBack(IntroT(timer, delay, duration));
}
