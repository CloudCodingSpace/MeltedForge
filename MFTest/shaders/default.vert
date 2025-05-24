#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 uv;

layout (location = 1) out vec3 oColor;
layout (location = 2) out vec2 oUv;

void main() {
    gl_Position = vec4(pos, 1.0);
    oColor = color;
    oUv = uv;
}