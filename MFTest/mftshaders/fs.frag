#version 450

#extension GL_EXT_scalar_block_layout : require

layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec2 st;

layout (set = 0, binding = 0) uniform sampler2D u_ColorAttachment;
layout (set = 0, binding = 1) uniform sampler2D u_DepthAttachment;

layout (set = 1, binding = 0, scalar) uniform CameraUBO {
    mat4 prevProj;
    mat4 prevView;
    mat4 proj;
    mat4 view;
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
        mat4 prevVP = camUbo.prevProj  * camUbo.prevView;
        mat4 invVP = inverse(camUbo.proj * camUbo.view);
        float depth = texture(u_DepthAttachment, uv).r;
        
        vec2 ndcXY = uv * 2.0 - 1.0;
        vec4 ndc = vec4(ndcXY, depth, 1.0);
        vec4 worldPos = invVP * ndc;
        worldPos /= worldPos.w;

        vec4 prevNDC = prevVP * worldPos;
        prevNDC /= prevNDC.w;

        vec2 prevNDCXY = prevNDC.xy;
        
        vec2 velocity = ndcXY - prevNDCXY;

        if(length(velocity) < 0.075) {
            FragColor = texture(u_ColorAttachment, uv);
            return;
        }

        int SAMPLES = pc.motionBlurSamples;
        vec3 color = vec3(0.0);
        for(int i = 0; i < SAMPLES; i++) {
            float t = float(i) / float(SAMPLES - 1);
            vec2 offset = velocity * t;
            if((1.0 - uv.x) < offset.x)
                offset.x = 0;
            else if((1.0 - uv.y) < offset.y)
                offset.y = 0;

            if(pc.showChromaticAberration == 1) {
                offset *= 0.5;
                color.r += texture(u_ColorAttachment, uv + offset).r;
                color.g += texture(u_ColorAttachment, uv).g;
                color.b += texture(u_ColorAttachment, uv - offset).b;
            } else {
                color += texture(u_ColorAttachment, uv + offset).rgb;
            }
        }
        color /= float(SAMPLES);
        FragColor = vec4(color, 1.0);
    } else if(pc.showChromaticAberration == 1) {
        vec4 color = vec4(1.0);
        vec2 offset = vec2(2e-3);
        if((1.0 - uv.x) < offset.x)
            offset.x = 0;
        else if((1.0 - uv.y) < offset.y)
            offset.y = 0;
        color.r = texture(u_ColorAttachment, uv + offset).r;
        color.g = texture(u_ColorAttachment, uv).g;
        color.b = texture(u_ColorAttachment, uv - offset).b;
        color.a = texture(u_ColorAttachment, uv).a;

        FragColor = color;
    } else {
        FragColor = texture(u_ColorAttachment, uv);
    }
}
