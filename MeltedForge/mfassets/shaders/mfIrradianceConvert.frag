#version 450

layout (location = 0) in vec3 oWorldPos;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform samplerCube u_Tex;

const float PI = 3.14159265359;

void main() {
    vec3 normal = normalize(oWorldPos);
    vec3 up = vec3(0, 1, 0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    vec3 irradiance = vec3(0.0);
    float dt = 0.025;
    float sampleCount = 0.0;
    for(float phi = 0; phi < 2.0 * PI; phi += dt) {
        for(float theta = 0; theta < 0.5 * PI; theta += dt) {
            vec3 tangent = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 dir = tangent.x * right + tangent.y * up + tangent.z * normal;
            irradiance += texture(u_Tex, dir).rgb * cos(theta) * sin(theta);
            sampleCount++;
        }
    }
    
    irradiance = PI * irradiance * (1.0 / sampleCount);

    outColor = vec4(irradiance, 1.0);
}