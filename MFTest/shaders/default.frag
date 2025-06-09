#version 450

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
    vec3 norm = normalize(oNormal);
    vec3 lightDir = normalize(ubo.lightPos - oFragPos);

    float diffuse = max(dot(norm, lightDir), 0.0);

    vec3 viewDir = normalize(ubo.camPos - oFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), ubo.specularFactor);

    outColor = texture(u_Tex, oUv) * (diffuse + ubo.ambientFactor + spec);
}