#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 uv;

layout(location = 0) uniform mat4 model;

layout(std140, binding = 0) uniform Camera {
	mat4 proj;
	mat4 view;
	vec3 camPos;
};

vec4 worldPos = model * vec4(pos, 1);

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outTangent;
layout(location = 3) out vec2 outuv;

void main() {
	gl_Position = proj * view * worldPos;
	outWorldPos = vec3(worldPos);
	outNormal = mat3(transpose(inverse(model))) * normal;
	outTangent = vec4(mat3(transpose(inverse(model))) * tangent.xyz, tangent.w);
	outuv = uv;
}
