#version 450

layout (location = 0) in vec3 oWorldPos;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D u_Tex;

vec2 SampleFromEnv(vec3 dir) {
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5;
    return uv;
}

void main() {
    vec3 dir = normalize(oWorldPos);
    outColor = vec4(texture(u_Tex, SampleFromEnv(dir)).rgb, 1.0);
}