#version 450

#include <mfshaderutils.glsl>

layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 oNormal;
layout (location = 2) in vec2 oUv;
layout (location = 3) in vec3 oFragPos;

layout (binding = 1) uniform LightUBO {
    vec3 lightPos;
    float ambientFactor;
    vec3 camPos;
    float specularFactor;
    vec3 lightColor;
    float lightIntensity;
} ubo;

layout (binding = 2) uniform sampler2D u_Tex;

void main() {
    outColor = texture(u_Tex, oUv) * vec4(mfComputePhongLighting(oNormal, 
                                    oFragPos, 
                                    ubo.lightPos - oFragPos, 
                                    ubo.camPos, 
                                    ubo.lightColor, 
                                    ubo.specularFactor, 
                                    ubo.ambientFactor, 
                                    ubo.lightIntensity, 
                                    true), 1.0);
}