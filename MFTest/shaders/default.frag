#version 450

layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 oColor;
layout (location = 2) in vec2 oUv;

layout (binding = 0) uniform sampler2D u_Tex;

void main() {
    outColor = texture(u_Tex, oUv);
}