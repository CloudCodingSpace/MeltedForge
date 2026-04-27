#version 450

layout (location = 0) in vec3 pos;

layout (location = 0) out vec3 oWorldPos;

layout(push_constant) uniform PushContants {
    mat4 proj;
    mat4 view;
} pc;

void main() {
    oWorldPos = pos;
    gl_Position = pc.proj * pc.view * vec4(pos, 1.0);
}