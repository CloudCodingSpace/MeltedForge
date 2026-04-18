#version 450
#extension GL_EXT_scalar_block_layout : require

#include <mfshaderutils.glsl>

layout(location = 0) out vec4 outColor;

layout (location = 4) in vec3 oNormal;
layout (location = 6) in vec3 oTangent;
layout (location = 7) in vec2 oUv;
layout (location = 8) in vec3 oFragPos;

layout (binding = 1, scalar) uniform LightUBO {
    vec3 lightPos;
    vec3 camPos;
    vec3 lightColor;
    float ambientFactor;
    float specularFactor;
    float lightIntensity;
    float isPoint;
} ubo;

layout (binding = 2) uniform sampler2D u_DiffuseTex;
layout (binding = 3) uniform sampler2D u_NormalTex;

void main() {
    vec3 texel = texture(u_NormalTex, oUv).rgb * 2.0 - 1.0;
    vec3 bitangent = normalize(cross(oNormal, oTangent));
    mat3 TBN = mat3(oTangent, bitangent, oNormal);
    vec3 normal = normalize(TBN * texel);

    MFPhongLightingInfo info;
    info.normal = normal;
    info.camPos = ubo.camPos;
    info.fragPos = oFragPos;
    info.lightColor = ubo.lightColor;
    info.lightDir = ubo.lightPos - oFragPos;
    info.lightIntensity = ubo.lightIntensity;
    info.ambientFactor = ubo.ambientFactor;
    info.specularFactor = ubo.specularFactor;
    info.isPoint = (ubo.isPoint == 1.0) ? true : false;

    outColor = pow(texture(u_DiffuseTex, oUv), vec4(2.2)) * vec4(mfComputePhongLighting(info), 1.0);

    outColor = outColor / (outColor + 1);
    outColor = pow(outColor, vec4(1.0/2.2));
}