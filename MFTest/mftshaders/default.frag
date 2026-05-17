#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

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
    float lightIntensity;
    int useNormalMap;
    int useAcesTonemapping;
} ubo;

layout (binding = 2) uniform sampler2D u_DiffuseTex;
layout (binding = 3) uniform sampler2D u_NormalTex;
layout (binding = 4) uniform sampler2D u_MetallicRoughness;
layout (binding = 5) uniform sampler2D u_EmissionTex;

layout (set = 1, binding = 0) uniform samplerCube u_Skybox;
layout (set = 1, binding = 1) uniform samplerCube u_IrradianceMap;
layout (set = 1, binding = 2) uniform samplerCube u_PrefilteredMap;
layout (set = 1, binding = 3) uniform sampler2D u_BrdfLUT;

void main() {
    vec4 albedo = texture(u_DiffuseTex, oUv);
    if(albedo.a < 0.5)
        discard;
    mfGammaCorrectedToLinear(albedo.rgb);

    vec3 normal;
    {
        vec3 texel = texture(u_NormalTex, oUv).rgb * 2.0 - 1.0;
        vec3 bitangent = normalize(cross(oNormal, oTangent));
        mat3 TBN = mat3(oTangent, bitangent, oNormal);
        normal = (ubo.useNormalMap == 1) ? normalize(TBN * texel) : oNormal;
    }

    vec4 metallicRoughness = texture(u_MetallicRoughness, oUv);
    vec4 emission = texture(u_EmissionTex, oUv);
    mfGammaCorrectedToLinear(metallicRoughness.rgb);
    mfGammaCorrectedToLinear(emission.rgb);

    MFPbrLightingInfo info;
    info.normal = normal;
    info.camPos = ubo.camPos;
    info.fragPos = oFragPos;
    info.lightColor = ubo.lightColor;
    info.lightPos = ubo.lightPos;
    info.roughness = metallicRoughness.g;
    info.metalness = metallicRoughness.b;
    info.lightIntensity = ubo.lightIntensity;
    info.albedoColor = albedo.rgb;
    info.emissionColor = emission.rgb;

    vec3 viewDir = normalize(info.camPos - info.fragPos);

    info.useIBLSamples = true;
    info.diffuseIrradianceSample = mfSampleFromIrradianceMap(u_IrradianceMap, normal);
    info.prefilteredSample = mfSampleFromPrefiltered(u_PrefilteredMap, viewDir, info.normal, info.roughness);
    info.brdfLutSample = mfSampleFromBRDFLUT(u_BrdfLUT, viewDir, normal, info.roughness);

    outColor = vec4(mfComputePbrLighting(info), 1.0);

    (ubo.useAcesTonemapping == 1) ? mfTonemapperAces(outColor.rgb) : mfTonemapperReinhard(outColor.rgb);
    mfGammaCorrect(outColor.rgb);
}