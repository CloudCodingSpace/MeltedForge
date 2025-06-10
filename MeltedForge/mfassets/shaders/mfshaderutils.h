#ifndef MFSHADER_UTILS
#define MFSHADER_UTILS

vec3 mfComputePhongLighting(vec3 normal, vec3 fragPos, vec3 lightPos, vec3 camPos, vec3 lightColor, float specularFactor, float ambientFactor) {
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);

    float diffuse = max(dot(norm, lightDir), 0.0);

    vec3 viewDir = normalize(camPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularFactor);
    return lightColor * (diffuse + spec + ambientFactor);
}

#endif