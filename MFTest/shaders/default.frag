#version 450

#include <mfshaderutils.glsl>

layout(location = 0) out vec4 outColor;

layout (location = 4) in mat3 oTBN_matrix;
layout (location = 7) in vec2 oUv;
layout (location = 8) in vec3 oFragPos;
layout (location = 9) in vec3 oNormal;

layout (binding = 1) uniform LightUBO {
    vec3 lightPos;
    float ambientFactor;
    vec3 camPos;
    float specularFactor;
    vec3 lightColor;
    float lightIntensity;
} ubo;

layout (binding = 2) uniform sampler2D u_DiffuseTex;
layout (binding = 3) uniform sampler2D u_NormalTex;

void main() {
    vec3 texel = texture(u_NormalTex, oUv).rgb * 2.0 - 1.0;
    vec3 normal = oTBN_matrix * texel;

    outColor = texture(u_DiffuseTex, oUv) * vec4(mfComputePhongLighting(oNormal, 
                                    oFragPos, 
                                    ubo.lightPos - oFragPos, 
                                    ubo.camPos, 
                                    ubo.lightColor, 
                                    ubo.specularFactor, 
                                    ubo.ambientFactor, 
                                    ubo.lightIntensity, 
                                    true), 1.0);

    outColor = outColor / (outColor + 1);
    outColor = pow(outColor, vec4(1.0/2.2));
}