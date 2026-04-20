#pragma once

#include "app/defs.h"

class UIAnim {
public:
    static float Clamp01(float v);
    static float EaseOutCubic(float t);
    static float EaseOutBack(float t);
    static uint32 AlphaRGBA(uint32 c, float aMul);
    static float Approach(float v, float target, float speed, float dt);
    static float Hover(float v, bool hover, float speed, float dt);
    static float IntroT(float timer, float delay, float duration);
    static float IntroAlpha(float timer, float delay, float duration);
    static float IntroMove(float timer, float delay, float duration);
};
