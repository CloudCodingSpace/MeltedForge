#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 1) out vec3 oNormal;
layout (location = 2) out vec2 oUv;
layout (location = 3) out vec3 oFragPos;

layout (binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;
} ubo;

layout (push_constant) uniform ModelData {
    mat4 model;
    mat4 normalMat;
} md;

void main() {
    gl_Position = ubo.proj * ubo.view * md.model * vec4(pos, 1.0);
    oFragPos = (md.model * vec4(pos, 1.0)).xyz;
    oNormal = mat3(md.normalMat) * normal;
    oUv = uv;
}
