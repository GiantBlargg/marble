#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 uv;

layout(location = 0) uniform mat4 model;

layout(std140, binding = 0) uniform Camera {
	mat4 proj;
	mat4 view;
	vec3 camPos;
};

vec4 worldPos = model * vec4(pos, 1);

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBitangent;
layout(location = 4) out vec2 outuv;

void main() {
	gl_Position = proj * view * worldPos;
	outWorldPos = vec3(worldPos);
	mat3 normalMatrix = mat3(transpose(inverse(model)));
	outNormal = normalMatrix * normal;
	outTangent = normalMatrix * tangent;
	outBitangent = normalMatrix * bitangent;
	outuv = uv;
}
