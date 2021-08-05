#version 460 core

layout(location = 0) in vec2 TexCoords;

layout(binding = 3) uniform sampler2D tex;

layout(location = 0) out vec4 outColour;

void main() { outColour = texture(tex, TexCoords); }