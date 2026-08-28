#version 450
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

layout (set = 0, binding = 0, scalar) uniform CameraUBO {
    mat4 prevViewProj;
    mat4 viewProj;
} camUbo;

layout (push_constant, scalar) uniform ModelData {
    mat4 model;
    mat3 normalMat;
    vec2 resolution;
} md;

void main() {
    vec4 worldPos = md.model * vec4(pos, 1.0);
    gl_Position = camUbo.viewProj * worldPos;
}