#version 450

#extension GL_EXT_scalar_block_layout : require

layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec2 uv;

layout (set = 0, binding = 0) uniform sampler2D u_ColorAttachment;
layout (set = 0, binding = 1) uniform sampler2D u_DepthAttachment;

layout (push_constant, scalar) uniform PushConstant {
    int showDepthAttachment;
    float zNear;
    float zFar;
} pc;

float LinearizeDepth(float d,float zNear,float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

void main() {
    if(pc.showDepthAttachment == 1) {
        float depth = texture(u_DepthAttachment, vec2(uv.x, 1.0 - uv.y)).r;
        depth = LinearizeDepth(depth, pc.zNear, pc.zFar);
        FragColor = vec4(vec3(depth), 1.0);
    } else {
        FragColor = texture(u_ColorAttachment, vec2(uv.x, 1.0 - uv.y));
    }
}