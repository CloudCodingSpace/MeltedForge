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
    int enableMotionBlur;
    int motionBlurSamples;
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
    } else if(pc.enableMotionBlur == 1) {
        mat4 invVP = inverse(camUbo.viewProj);
        float depth = texture(u_DepthAttachment, uv).r;
        
        vec2 ndcXY = uv * 2.0 - 1.0;
        vec4 ndc = vec4(ndcXY, depth, 1.0);
        vec4 worldPos = invVP * ndc;
        worldPos /= worldPos.w;

        vec4 prevNDC = camUbo.prevViewProj * worldPos;
        prevNDC /= prevNDC.w;

        vec2 prevNDCXY = prevNDC.xy;
        
        vec2 velocity = ndcXY - prevNDCXY;
        
        float speed = length(velocity);

        float maxVelocity = 0.05;

        if(speed > maxVelocity) {
            velocity = normalize(velocity) * maxVelocity;
        } else if(speed < 0.002) {
            FragColor = texture(u_ColorAttachment, uv);
            return;
        }

        int SAMPLES = pc.motionBlurSamples;
        vec3 color = vec3(0.0);
        for(int i = 0; i < SAMPLES; i++) {
            float t = float(i) / float(SAMPLES - 1);
            vec2 offset = velocity * (t - 0.5);
            vec2 sampleUv = clamp(uv + offset, vec2(0.001), vec2(0.999));

            if(pc.showChromaticAberration == 1) {
                offset *= 0.5;
                color += mfChromaticAberrate(u_ColorAttachment, uv, offset).rgb;
            } else {
                color += texture(u_ColorAttachment, uv + offset).rgb;
            }
        }
        color /= float(SAMPLES);
        FragColor = vec4(color, 1.0);
    } else if(pc.showChromaticAberration == 1) {
        vec4 color = vec4(1.0);
        vec2 offset = vec2(2e-3);

        FragColor = mfChromaticAberrate(u_ColorAttachment, uv, offset);
    } else {
        FragColor = texture(u_ColorAttachment, uv);
    }
}
