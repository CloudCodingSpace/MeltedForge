#ifndef MFSHADER_UTILS
#define MFSHADER_UTILS

struct MFPhongLightingInfo {
    vec3 normal;
    vec3 fragPos;
    vec3 lightDir;
    vec3 camPos;
    vec3 lightColor;
    float specularFactor;
    float ambientFactor;
    float lightIntensity;
    bool isPoint;
};

vec3 mfComputePhongLighting(MFPhongLightingInfo info) {
    vec3 norm = normalize(info.normal);
    vec3 dir = normalize(info.lightDir);

    float diffuse = max(dot(norm, dir), 0.0);

    vec3 viewDir = normalize(info.camPos - info.fragPos);
    vec3 reflectDir = reflect(-dir, norm);

    float spec = 0.0;
    if(diffuse > 0.0) {
        spec = pow(max(dot(viewDir, reflectDir), 0.0), info.specularFactor);
    }

    vec3 color = info.lightColor * (diffuse + spec + info.ambientFactor);

    if(info.isPoint) {
        float dist = dot(info.lightDir,info. lightDir);
        float attenuation = 1.0 / dist;
        color *= attenuation;
    }
    return color * info.lightIntensity;
}

#endif