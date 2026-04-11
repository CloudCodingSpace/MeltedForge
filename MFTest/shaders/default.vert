#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;

layout (location = 4) out mat3 oTBN_matrix;
layout (location = 7) out vec2 oUv;
layout (location = 8) out vec3 oFragPos;
layout (location = 9) out vec3 oNormal;

layout (binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;
} ubo;

layout (push_constant) uniform ModelData {
    mat4 model;
    mat4 normalMat;
} md;

void main() {
    vec4 worldPos = md.model * vec4(pos, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    vec3 N = normalize(mat3(md.normalMat) * normal);
    vec3 T = normalize(mat3(md.normalMat) * tangent);
    vec3 B = normalize(cross(N, tangent));
    oTBN_matrix = mat3(T, B, N);

    oFragPos = worldPos.xyz;
    oUv = uv;
    oNormal = normalize(normal);
}
