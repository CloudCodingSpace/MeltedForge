#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include <mfshaderutils.glsl>

layout(location = 0) out vec4 outColor;

in VS_IN {
    layout (location = 4) vec3 oNormal;
    layout (location = 6) vec3 oTangent;
    layout (location = 7) vec3 oBitangent;
    layout (location = 8) vec2 oUv;
    layout (location = 9) vec3 oFragPos;
} vi;

layout (set = 0, binding = 1, scalar) uniform LightUBO {
    vec3 lightPos;
    vec3 camPos;
    vec3 lightColor;
    float lightIntensity;
    float iblDiffuseStrength;
    float iblSpecularStrength;
    int useNormalMap;
    int useAoMap;
    int useIBL;
} ubo;

layout (set = 2, binding = 0) uniform sampler2D u_DiffuseTex;
layout (set = 2, binding = 1) uniform sampler2D u_NormalTex;
layout (set = 2, binding = 2) uniform sampler2D u_MetallicRoughness;
layout (set = 2, binding = 3) uniform sampler2D u_EmissionTex;
layout (set = 2, binding = 4) uniform sampler2D u_AoMap;

layout (set = 1, binding = 0) uniform samplerCube u_Skybox;
layout (set = 1, binding = 1) uniform samplerCube u_IrradianceMap;
layout (set = 1, binding = 2) uniform samplerCube u_PrefilteredMap;
layout (set = 1, binding = 3) uniform sampler2D u_BrdfLUT;

float LinearizeDepth(float d,float zNear,float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

void main() {
    vec4 albedo = texture(u_DiffuseTex, vi.oUv);
    if(albedo.a < 0.5)
        discard;
    mfGammaCorrectedToLinear(albedo.rgb);

    vec3 normal;
    {
        vec3 texel = texture(u_NormalTex, vi.oUv).rgb * 2.0 - 1.0;
        mat3 TBN = mat3(vi.oTangent, vi.oBitangent, vi.oNormal);
        normal = (ubo.useNormalMap == 1) ? normalize(TBN * texel) : vi.oNormal;
    }

    vec4 metallicRoughness = texture(u_MetallicRoughness, vi.oUv);
    vec4 emission = texture(u_EmissionTex, vi.oUv);
    mfGammaCorrectedToLinear(emission.rgb);

    MFPbrLightingInfo info;
    info.normal = normal;
    info.camPos = ubo.camPos;
    info.fragPos = vi.oFragPos;
    info.lightColor = ubo.lightColor;
    info.lightDir = vi.oFragPos - ubo.lightPos;
    info.roughness = metallicRoughness.g;
    info.metalness = metallicRoughness.b;
    info.lightIntensity = ubo.lightIntensity;
    info.albedoColor = albedo.rgb;

    vec3 color = mfComputePbrLighting(info);
    // Point lighting radiance
    {
        float distance = dot(-info.lightDir, -info.lightDir);
        float attenuation = 1.0 / distance;
        vec3 radiance = info.lightColor * info.lightIntensity * attenuation;
        color *= radiance;
    }

    color += emission.rgb;

    if(ubo.useIBL == 1) {
        vec3 viewDir = normalize(info.camPos - info.fragPos);

        MFIBLInfo iblInfo;
        iblInfo.ambientOcclusion = (ubo.useAoMap == 1) ? texture(u_AoMap, vi.oUv).r : 1.0;
        iblInfo.iblDiffuseStrength = ubo.iblDiffuseStrength;
        iblInfo.iblSpecularStrength = ubo.iblSpecularStrength;
        iblInfo.diffuseIrradianceSample = mfSampleFromIrradianceMap(u_IrradianceMap, normal);
        iblInfo.prefilteredSample = mfSampleFromPrefiltered(u_PrefilteredMap, viewDir, info.normal, info.roughness);
        iblInfo.brdfLutSample = mfSampleFromBRDFLUT(u_BrdfLUT, viewDir, normal, info.roughness);
    
        color += mfComputeIBL(iblInfo, info);
    }

    outColor = vec4(mix(color, vec3(0.78), vec3(LinearizeDepth(gl_FragCoord.z, 0.01, 1000.0) / 256)), 1.0);

    mfTonemapperAces(outColor.rgb);
    mfGammaCorrect(outColor.rgb);
}