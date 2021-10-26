#version 460 core

layout(location = 4) in sample vec2 uv;

layout(std140, binding = 1) uniform Material {
	vec4 albedoFactor;
	vec3 emissiveFactor;
	float metalFactor;
	float roughFactor;
	float reflectionLevels;
	bool has_albedo_texture;
	bool has_metal_rough_texture;
	bool has_normal_texture;
	bool has_occlusion_texture;
	bool has_emissive_texture;
	float alpha_depth_cutoff;
};
layout(binding = 3) uniform sampler2D albedoTex;

void main() {
	float alpha;
	if (has_albedo_texture)
		alpha = albedoFactor.a * texture(albedoTex, uv).a;
	else
		alpha = albedoFactor.a;

	if (alpha < alpha_depth_cutoff)
		discard;
}