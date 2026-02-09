#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in vec4 aColor;
out vec2 vUV; out vec4 vColor;
uniform vec2 uView; // píxeles
void main(){ vUV=aUV; vColor=aColor; vec2 p = (aPos/uView)*2.0 - 1.0;
  gl_Position = vec4(p.x, -p.y, 0, 1); }