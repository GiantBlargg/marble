#version 460 core

layout(location = 0) in vec3 TexCoords;

layout(location = 0) out vec4 outColour;

void main() { outColour = vec4((TexCoords + 1) / 2, 1); }