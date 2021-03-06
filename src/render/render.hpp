#pragma once

#include <optional>

#include "core.hpp"
#include "standard_mesh.hpp"

namespace Render {

struct MaterialPBR {
	vec4 albedoFactor;
	std::optional<TextureHandle> albedoTexture;
	float metalFactor;
	float roughFactor;
	std::optional<TextureHandle> metalRoughTexture;
	std::optional<TextureHandle> normalTexture;
	std::optional<TextureHandle> occlusionTexture;
	vec3 emissiveFactor;
	std::optional<TextureHandle> emissiveTexture;
	enum class AlphaMode { Opaque, Masked, Blend };
	AlphaMode alphaMode;
	float alphaCutoff = 0.5f;
	bool doubleSided = false;
};

class Render : public Core {
	const int skyboxSize = 4096;
	const int irradianceSize = 64;
	const int reflectionSize = 512;
	const int reflectionLevels = 5;
	const int reflectionBRDFSize = 256;

	SurfaceHandle skybox;
	TextureHandle skyboxCubemap;
	TextureHandle irradiance;
	TextureHandle reflection;
	TextureHandle reflectionBRDF;

	GLuint zero_buffer;

	void render_cubemap(Shader::Type type, RenderOrder order, GLuint cubemap, GLsizei width);

  public:
	Render(void (*glGetProcAddr(const char*))());
	Render(const Render&) = delete;

	MaterialHandle create_pbr_material(MaterialPBR);

	void set_skybox_material(MaterialHandle material, bool update = true) {
		surface_set_material(skybox, material);
		if (update)
			update_skybox();
	}
	void set_skybox_rect_texture(TextureHandle, bool update = true);
	void set_skybox_cube_texture(TextureHandle, bool update = true);

	void update_skybox();

	MeshHandle standard_mesh_create(StandardMesh mesh);
};

} // namespace Render