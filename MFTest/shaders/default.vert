#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;

layout (location = 4) out mat3 oTBN_matrix;
layout (location = 7) out vec2 oUv;
layout (location = 8) out vec3 oFragPos;

layout (binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;
} ubo;

layout (push_constant) uniform ModelData {
    mat4 model;
    mat4 normalMat;
} md;

void main() {
    gl_Position = ubo.proj * ubo.view * md.model * vec4(pos, 1.0);
    
    vec3 N = normalize(mat3(md.normalMat) * normal);
    vec3 T = normalize(mat3(md.model) * tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(N, tangent));
    oTBN_matrix = mat3(T, B, N);

    oFragPos = (md.model * vec4(pos, 1.0)).xyz;
    oUv = uv;
}
