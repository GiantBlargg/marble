#pragma once

#include "core.hpp"
#include <optional>

namespace Render {

struct MaterialPBR {
	vec4 albedoFactor;
	std::optional<TextureHandle> albedoTexture;
	float metalFactor;
	float roughFactor;
	std::optional<TextureHandle> metalRoughTexture;
	vec3 emissiveFactor;
	std::optional<TextureHandle> emissiveTexture;
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

	class StandardMesh {
	  private:
		friend class Render;
		std::vector<float> vertex_data;
		bool normal, tangent;
		size_t vertex_count;
		size_t tex_coord_count, colour_count;
		size_t stride;

	  public:
		StandardMesh() : StandardMesh(0, false, false, 0, 0){};
		StandardMesh(
			size_t vertex_count, bool has_normal, bool has_tangent, size_t tex_coord_count, size_t colour_count) {
			resize(vertex_count, has_normal, has_tangent, tex_coord_count, colour_count);
		}
		void resize(size_t vertex, bool has_normal, bool has_tangent, size_t tex_coord, size_t colour);
		// Data is zeroed after resize

		void set_position(int vertex, vec3 value);

		std::vector<uint32_t> indices;
	};

	MeshHandle standard_mesh_create(StandardMesh mesh);
};

} // namespace Render