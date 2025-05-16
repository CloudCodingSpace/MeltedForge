#version 450

layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 oColor;

void main() {
    outColor = vec4(oColor, 1.0);
}