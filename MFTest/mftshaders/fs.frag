#version 450

layout (set = 0, binding = 0) uniform sampler2D u_ColorAttachment;
layout (set = 0, binding = 1) uniform sampler2D u_DepthAttachment;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = texture(u_ColorAttachment, vec2(uv.x, 1.0 - uv.y));
}