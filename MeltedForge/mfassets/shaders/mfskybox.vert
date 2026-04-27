#version 450

layout (location = 0) in vec3 pos;

layout (location = 0) out vec3 oTexCoords;

layout (push_constant) uniform PushConstant {
    mat4 proj;
    mat4 view;
} pc;

void main() {
    oTexCoords = pos;

    mat4 noRotView = mat4(mat3(pc.view));
    gl_Position = pc.proj * noRotView * vec4(pos, 1.0);
    gl_Position = gl_Position.xyww;
}