#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 1) out vec3 oNormal;
layout (location = 2) out vec2 oUv;

layout (binding = 3) uniform UBO {
    mat4 proj;
    mat4 view;
    mat4 model;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
    oNormal = normal;
    oUv = uv;
}