#ifndef MFSHADER_UTILS
#define MFSHADER_UTILS

////////////////////////             Phong lighting                /////////////////////////////////
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

vec3 mfComputePhongLighting(in MFPhongLightingInfo info) {
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
        float dist = dot(info.lightDir, info.lightDir);
        float attenuation = 1.0 / dist;
        color *= attenuation;
    }
    return color * info.lightIntensity;
}

////////////////////////             Pbr lighting                /////////////////////////////////
const float PI = 3.14159265358979323846;

struct MFPbrLightingInfo {
    vec3 normal;
    float roughness, metalness, lightIntensity;
    vec3 lightColor;
    vec4 diffuseIrradianceSample;
    vec3 albedoColor;
    vec3 camPos;
    vec3 fragPos;
    vec3 lightPos;
    bool useIrradianceSample;
};

float _mfGeoSmithApprox(float x, float roughness) {
    float k = pow(roughness + 1, 2) / 8.0;
    return x / (x * (1 - k) + k);
}

vec3 _mfFresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

vec3 mfComputePbrLighting(in MFPbrLightingInfo info) {
    float roughness = max(info.roughness, 0.03);

    vec3 N = normalize(info.normal);
    vec3 V = normalize(info.camPos - info.fragPos);
    vec3 L = normalize(info.lightPos - info.fragPos);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float VdotH = max(dot(V, H), 0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, info.albedoColor, info.metalness);
    vec3 F = info.useIrradianceSample ? _mfFresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness) : (F0 + (1.0 - F0) * pow(1- VdotH, 5));

    float a = pow(roughness, 2);
    float a2 = a * a;
    float D = a2 / (PI * pow(pow(NdotH, 2) * (a2 - 1.0) + 1, 2));
    float G = _mfGeoSmithApprox(NdotL, roughness) * _mfGeoSmithApprox(NdotV, roughness);

    vec3 specular = (D * F * G) / (4 * NdotL * NdotV + 1e-3);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - info.metalness;

    float distance2 = dot(info.lightPos - info.fragPos, info.lightPos - info.fragPos);
    float attenuation = 1.0 / distance2;
    vec3 radiance = info.lightColor * info.lightIntensity * attenuation;

    vec3 irradiance = info.useIrradianceSample ? info.diffuseIrradianceSample.rgb : vec3(1.0);
    vec3 diffuse = info.albedoColor / PI;
    diffuse *= irradiance;
    vec3 LO = (kD * diffuse + specular) * radiance * NdotL;
    vec3 ambient = kD * diffuse;

    return vec3(LO + ambient);
}

#endif