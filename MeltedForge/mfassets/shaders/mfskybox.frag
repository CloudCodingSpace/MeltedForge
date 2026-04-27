#version 450

layout (location = 0) in vec3 oTexCoords;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform samplerCube u_Skybox;

void main() {
    outColor = texture(u_Skybox, oTexCoords);
}