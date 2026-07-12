#ifndef MFSHADER_UTILS
#define MFSHADER_UTILS

////////////////////////             Phong lighting                /////////////////////////////////
struct MFPhongLightingInfo {
    vec3 normal;
    vec3 fragPos;
    vec3 lightDir;
    vec3 camPos;
    float shininess;
    float ambientFactor;
    bool isPoint;
};

float mfComputePhongLightingFactor(in MFPhongLightingInfo info) {
    vec3 norm = normalize(info.normal);
    vec3 dir = normalize(info.lightDir);

    float diffuse = max(dot(norm, dir), 0.0);

    vec3 viewDir = normalize(info.camPos - info.fragPos);
    vec3 reflectDir = reflect(-dir, norm);

    float spec = 0.0;
    if(diffuse > 0.0) {
        spec = pow(max(dot(viewDir, reflectDir), 0.0), info.shininess);
    }

    float phongFactor = diffuse + spec + info.ambientFactor;

    if(info.isPoint) {
        float dist = dot(info.lightDir, info.lightDir);
        float attenuation = 1.0 / dist;
        phongFactor *= attenuation;
    }
    return phongFactor;
}

////////////////////////             Pbr lighting                /////////////////////////////////
const float PI = 3.14159265358979323846;
const float TAU = 6.28318530718;

struct MFPbrLightingInfo {
    vec3 normal;
    float roughness, metalness, lightIntensity;
    vec3 lightColor;
    vec3 albedoColor;
    vec3 camPos;
    vec3 fragPos;
    vec3 lightPos;
};

struct MFIBLInfo {
    float ambientOcclusion;
    float iblDiffuseStrength;
    float iblSpecularStrength;
    vec4 diffuseIrradianceSample;
    vec4 prefilteredSample;
    vec2 brdfLutSample;
};

vec4 mfSampleFromIrradianceMap(samplerCube map, vec3 normal) {
    return texture(map, normal);
}

vec2 mfSampleFromBRDFLUT(sampler2D map, vec3 viewDir, vec3 normal, float roughness) {
    return texture(map, vec2(max(dot(viewDir, normal), 0.0), roughness)).rg;
}

vec4 mfSampleFromPrefiltered(samplerCube map, vec3 viewDir, vec3 normal, float roughness) {
    const float MAX_REFLECTION_LOD = 4.0;
    return textureLod(map, normalize(reflect(-viewDir, normal)),  roughness * MAX_REFLECTION_LOD) * 0.5;
}

float _mfGeoSmithApprox(float x, float roughness) {
    float k = pow(roughness + 1, 2) / 8.0;
    return x / (x * (1 - k) + k);
}

vec3 _mfFresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 _mfFresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 mfComputePbrLighting(in MFPbrLightingInfo info) {
    float roughness = max(info.roughness, 0.1);

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
    vec3 F = _mfFresnelSchlick(VdotH, F0);

    float a = pow(roughness, 2);
    float a2 = a * a;
    float D = a2 / (PI * pow(pow(NdotH, 2) * (a2 - 1.0) + 1, 2));
    float G = _mfGeoSmithApprox(NdotL, roughness) * _mfGeoSmithApprox(NdotV, roughness);

    vec3 specular = (D * F * G) / (4 * NdotL * NdotV + 1e-3);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - info.metalness;

    float distance = dot(info.lightPos - info.fragPos, info.lightPos - info.fragPos);
    float attenuation = 1.0 / distance;
    vec3 radiance = info.lightColor * info.lightIntensity * attenuation;
    
    vec3 diffuse = info.albedoColor / PI;

    return (kD * diffuse + specular) * radiance * NdotL; // LO
}

vec3 mfComputeIBL(in MFIBLInfo info, in MFPbrLightingInfo lightingInfo) {
    float roughness = max(lightingInfo.roughness, 0.1);

    vec3 N = normalize(lightingInfo.normal);
    vec3 V = normalize(lightingInfo.camPos - lightingInfo.fragPos);
    vec3 L = normalize(lightingInfo.lightPos - lightingInfo.fragPos);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float VdotH = max(dot(V, H), 0.0);

    vec3 F0 = vec3(0.04);
    vec3 F = _mfFresnelSchlickRoughness(NdotV, F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - lightingInfo.metalness;

    vec3 irradiance = info.diffuseIrradianceSample.rgb;
    vec3 diffuse = irradiance * lightingInfo.albedoColor * info.iblDiffuseStrength;
    vec3 prefilteredColor = info.prefilteredSample.rgb;
    vec3 specular = prefilteredColor * (F * info.brdfLutSample.x + info.brdfLutSample.y) * info.iblSpecularStrength;
    vec3 ambient = (kD * diffuse + specular) * info.ambientOcclusion;

    return ambient;
}

////////////////////////             Gamma and Tonemappers                /////////////////////////////////

void mfGammaCorrectedToLinear(inout vec3 color) {
    color = pow(color, vec3(2.2));
}

void mfGammaCorrect(inout vec3 color) {
    color = pow(color, vec3(1.0/2.2));
}

void mfTonemapperReinhard(inout vec3 color) {
    color = color / (color + vec3(1.0));
}

void mfTonemapperAces(inout vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    x = clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 _mfTonemapperUncharted2Tonemap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

void mfTonemapperUncharted2Tonemap(inout vec3 color) {
    const float W = 11.2;
    float exposureBias = 2.0;
    vec3 curr = _mfTonemapperUncharted2Tonemap(exposureBias * color);
    vec3 whiteScale = 1.0 / _mfTonemapperUncharted2Tonemap(vec3(W));
    color = curr * whiteScale;
}

#endif