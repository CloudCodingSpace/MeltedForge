#version 450

#include <mfshaderutils.h>

layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 oNormal;
layout (location = 2) in vec2 oUv;
layout (location = 3) in vec3 oFragPos;

layout (binding = 0) uniform sampler2D u_Tex;

layout (binding = 1) uniform LightUBO {
    vec3 lightPos;
    float ambientFactor;
    vec3 camPos;
    float specularFactor;
} ubo;

void main() {
    outColor = texture(u_Tex, oUv) * vec4(mfComputePhongLighting(oNormal, 
                                    oFragPos, 
                                    ubo.lightPos, 
                                    ubo.camPos, 
                                    vec3(1, 1, 1), 
                                    ubo.specularFactor, 
                                    ubo.ambientFactor), 1.0);
}