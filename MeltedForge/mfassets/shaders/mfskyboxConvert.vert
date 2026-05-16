#version 450

layout (location = 0) in vec3 pos;

layout (location = 0) out vec3 oWorldPos;

layout(push_constant) uniform PushContants {
    mat4 vp;
    float roughness;
} pc;

void main() {
    oWorldPos = pos;
    gl_Position = pc.vp * vec4(pos, 1.0);
}