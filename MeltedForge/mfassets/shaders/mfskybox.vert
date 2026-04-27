#version 450

layout (location = 0) in vec3 pos;

layout (location = 0) out vec3 oTexCoords;

layout (push_constant) uniform PushConstant {
    mat4 viewProj;
    mat4 model;
} pc;

void main() {
    oTexCoords = pos;

    gl_Position = pc.viewProj * pc.model * vec4(pos, 1.0);
    gl_Position = gl_Position.xyww;
}