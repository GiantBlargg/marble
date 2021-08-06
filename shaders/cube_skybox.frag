#version 460 core

layout(location = 0) in vec3 TexCoords;

layout(binding = 3) uniform samplerCube cubeSkybox;

layout(location = 0) out vec4 outColour;

void main() { outColour = texture(cubeSkybox, normalize(TexCoords)); }