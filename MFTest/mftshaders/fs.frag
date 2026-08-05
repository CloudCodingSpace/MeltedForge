#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : require

#include <mfshaderutils.glsl>

layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec2 st;

layout (set = 0, binding = 0) uniform sampler2D u_ColorAttachment;
layout (set = 0, binding = 1) uniform sampler2D u_DepthAttachment;

layout (set = 1, binding = 0, scalar) uniform CameraUBO {
    mat4 prevViewProj;
    mat4 viewProj;
} camUbo;

layout (push_constant, scalar) uniform PushConstant {
    int showDepthAttachment;
    int showChromaticAberration;
    float zNear;
    float zFar;
} pc;

float LinearizeDepth(float d,float zNear,float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

void main() {
    vec2 uv = vec2(st.x, 1.0 - st.y);
    if(pc.showDepthAttachment == 1) {
        float depth = texture(u_DepthAttachment, uv).r;
        depth = LinearizeDepth(depth, pc.zNear, pc.zFar) * 25 / pc.zFar; // * 25 / pc.zFar only for demonstration
        FragColor = vec4(vec3(depth), 1.0);
        return;
    } 
    // Motion blur
    /* else if(pc.enableMotionBlur == 1) {
        FragColor = mfScreenSpaceMotionBlur(u_ColorAttachment, texture(u_DepthAttachment, uv).r, pc.motionBlurSamples, pc.showChromaticAberration, uv, camUbo.viewProj, camUbo.prevViewProj);
    } */ 
    else if(pc.showChromaticAberration == 1) {
        vec4 color = vec4(1.0);
        vec2 offset = vec2(2e-3);

        FragColor = mfChromaticAberrate(u_ColorAttachment, uv, offset);
    } else {
        FragColor = texture(u_ColorAttachment, uv);
    }
}
