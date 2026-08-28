#version 450
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out VS_OUT {
    layout (location = 4) out vec3 oNormal;
    layout (location = 6) out vec3 oTangent;
    layout (location = 7) out vec3 oBitangent;
    layout (location = 8) out vec2 oUv;
    layout (location = 9) out vec3 oFragPos;
} vo;

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
    
    vo.oNormal = normalize(md.normalMat * normal);
    vo.oTangent = normalize(md.normalMat * tangent);
    vo.oBitangent = normalize(md.normalMat * bitangent);
    vo.oFragPos = worldPos.xyz;
    vo.oUv = uv;
}
