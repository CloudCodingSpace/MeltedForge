#version 450
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;

layout (location = 4) out vec3 oNormal;
layout (location = 6) out vec3 oTangent;
layout (location = 7) out vec2 oUv;
layout (location = 8) out vec3 oFragPos;

layout (binding = 0, scalar) uniform UBO {
    mat4 proj;
    mat4 view;
} ubo;

layout (push_constant, scalar) uniform ModelData {
    mat4 model;
    mat3 normalMat;
} md;

void main() {
    vec4 worldPos = md.model * vec4(pos, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    oNormal = normalize(md.normalMat * normal);
    oTangent = normalize(md.normalMat * tangent);
    oFragPos = worldPos.xyz;
    oUv = uv;
}
