#version 330 core

in vec2 uv;
out vec4 FragColor;

uniform float uFade;
uniform float uEdge;
uniform vec2 uView;

float hash12(vec2 p)
{
    // https://www.shadertoy.com/view/4djSRW (referencia)
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main()
{
    float f = clamp(uFade, 0.0, 1.0);

    if (f <= 0.0001) {
        FragColor = vec4(0.0);
        return;
    }

    float bottomBias = (1.0 - uv.y) * 0.35;

    float t = clamp(f * (1.0 + bottomBias), 0.0, 1.0);
    float n = hash12(floor(gl_FragCoord.xy));
    float a = smoothstep(n - uEdge, n + uEdge, t);

    FragColor = vec4(0.0, 0.0, 0.0, a);
}
