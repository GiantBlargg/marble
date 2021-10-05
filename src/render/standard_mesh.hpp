#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace Render {

using namespace glm;

#define STANDARD_MESH_VERTEX_FORMAT                                                                                    \
	STANDARD_MESH_VERTEX_FEILD(position, 3)                                                                            \
	STANDARD_MESH_VERTEX_FEILD(normal, 3)                                                                              \
	STANDARD_MESH_VERTEX_FEILD(tangent, 3)                                                                             \
	STANDARD_MESH_VERTEX_FEILD(bitangent, 3)                                                                           \
	STANDARD_MESH_VERTEX_FEILD(tex_coord_0, 2)                                                                         \
	STANDARD_MESH_VERTEX_FEILD(tex_coord_1, 2)                                                                         \
	STANDARD_MESH_VERTEX_FEILD(tex_coord_2, 2)                                                                         \
	STANDARD_MESH_VERTEX_FEILD(tex_coord_3, 2)                                                                         \
	STANDARD_MESH_VERTEX_FEILD(colour_0, 4)                                                                            \
	STANDARD_MESH_VERTEX_FEILD(colour_1, 4)

class StandardMesh {
  private:
	friend class Render;

  public:
	struct Format {
#define STANDARD_MESH_VERTEX_FEILD(name, size) bool has_##name;
		STANDARD_MESH_VERTEX_FORMAT
#undef STANDARD_MESH_VERTEX_FEILD
	};

  private:
	std::vector<float> vertex_data;
	size_t vertex_count = 0;
	Format format;

#define STANDARD_MESH_VERTEX_FEILD(name, size) size_t offset_##name;
	STANDARD_MESH_VERTEX_FORMAT
#undef STANDARD_MESH_VERTEX_FEILD

	size_t stride = 0;

  public:
	StandardMesh() : StandardMesh(0, {false, false, false, false, false, false, false, false, false, false}){};
	StandardMesh(size_t vertex_count, Format format) { resize(vertex_count, format); }
	void resize(size_t vertex, Format);
	// TODO: Data is zeroed after resize
	void resize(size_t vertex) { resize(vertex, format); }
	void resize(Format format) { resize(vertex_count, format); }

	size_t get_vertex_count() { return vertex_count; }
	const Format& get_format() { return format; }

#define STANDARD_MESH_VERTEX_FEILD(name, size)                                                                         \
	vec##size& name(int vertex) {                                                                                      \
		return *reinterpret_cast<vec##size*>(vertex_data.data() + stride * vertex + offset_##name);                    \
	}
	STANDARD_MESH_VERTEX_FORMAT
#undef STANDARD_MESH_VERTEX_FEILD

#define ACCESSOR_HELPER(name, set)                                                                                     \
	case set:                                                                                                          \
		return name##_##set(vertex);
	vec2& tex_coord(int set, int vertex) {
		switch (set) {
			ACCESSOR_HELPER(tex_coord, 0);
			ACCESSOR_HELPER(tex_coord, 1);
			ACCESSOR_HELPER(tex_coord, 2);
			ACCESSOR_HELPER(tex_coord, 3);
		}
	}
	vec4& colour(int set, int vertex) {
		switch (set) {
			ACCESSOR_HELPER(colour, 0);
			ACCESSOR_HELPER(colour, 1);
		}
	}
#undef ACCESSOR_HELPER

	std::vector<uint32_t> indices;

	void deindex();
	void reindex();

	void gen_normals();
	void gen_tangents();
};

} // namespace Render