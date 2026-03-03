#version 330 core

in vec2 uv;
out vec4 FragColor;

uniform float uFade;
uniform float uEdge;
uniform vec2 uView;

uniform vec3  uColor;
uniform float uCellPx;

float hash2(ivec2 p){
    uint x = uint(p.x)*374761393u ^ uint(p.y)*668265263u;
    x = (x ^ (x>>13)) * 1274126177u;
    x ^= x >> 16u;
    return float(x & 1023u) / 1023.0;
}

void main()
{
    float f = clamp(uFade, 0.0, 1.0);
    if (f <= 0.0001) { FragColor = vec4(0.0); return; }

    float cellPx = max(2.0, uCellPx);

    vec2 cellf = gl_FragCoord.xy / cellPx;
    ivec2 cellId = ivec2(floor(cellf));

    vec2 cell = fract(cellf);
    vec2 p = cell - vec2(0.5);
    float r = length(p);

    float radius  = 0.35;
    float feather = 0.30;
    float dotA = 1.0 - smoothstep(radius, radius + feather, r);

    //aparicion desde abajo
    float n = hash2(cellId);
    float fromBottom = 1.0 - clamp(gl_FragCoord.y / max(1.0, uView.y), 0.0, 1.0); // 1 abajo
    float t = clamp(f * (1.0 + 0.35 * fromBottom), 0.0, 1.0);

    float cellA = smoothstep(n - uEdge, n + uEdge, t);

    float fill = smoothstep(0.70, 1.00, f);
    float shapeA = mix(dotA, 1.0, fill);

    float a = cellA * shapeA;
    if (a <= 0.001) { FragColor = vec4(0.0); return; }

    float vn = hash2(cellId + ivec2(17,91)) * 2.0 - 1.0; // [-1,1]
    float k  = 0.15;
    vec3 col = clamp(uColor * (1.0 + k * vn), 0.0, 1.0);

    FragColor = vec4(col, a);
}