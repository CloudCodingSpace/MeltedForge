#ifndef MFSHADER_UTILS
#define MFSHADER_UTILS

vec3 mfComputePhongLighting(vec3 normal, vec3 fragPos, vec3 lightDir, vec3 camPos, vec3 lightColor, float specularFactor, float ambientFactor, float lightIntensity, bool isPoint) {
    vec3 norm = normalize(normal);
    float attenuation = 1.0 / dot(lightDir, lightDir);
    vec3 dir = normalize(lightDir);

    float diffuse = max(dot(norm, dir), 0.0);

    vec3 viewDir = normalize(camPos - fragPos);
    vec3 reflectDir = reflect(-dir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularFactor);

    vec3 color = lightColor * (diffuse + spec + ambientFactor);

    return (isPoint) ? (color * attenuation * lightIntensity) : color;
}

#endif