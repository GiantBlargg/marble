#pragma once

#include <array>
#include <glm/gtc/integer.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Render {

using namespace glm;

#define RESOURCE_CONTAINER(T, name, Self)                                                                              \
  private:                                                                                                             \
	std::vector<T> name##_dense;                                                                                       \
	std::vector<std::pair<size_t, size_t>> name##_lookup;                                                              \
	std::vector<size_t> name##_reverse;                                                                                \
	std::vector<size_t> name##_recycled;                                                                               \
	void name##_setup(size_t handle);                                                                                  \
	void name##_cleanup(size_t handle);                                                                                \
	size_t name##_insert_unsafe(const T& item) {                                                                       \
		size_t handle = 0;                                                                                             \
		if (name##_recycled.empty()) {                                                                                 \
			handle = name##_lookup.size();                                                                             \
			name##_lookup.emplace_back();                                                                              \
		} else {                                                                                                       \
			handle = name##_recycled.back();                                                                           \
			name##_recycled.pop_back();                                                                                \
		}                                                                                                              \
		name##_dense.push_back(item);                                                                                  \
		name##_reverse.push_back(handle);                                                                              \
		name##_lookup[handle].first = name##_dense.size() - 1;                                                         \
		name##_lookup[handle].second = 0;                                                                              \
		name##_setup(handle);                                                                                          \
		return handle;                                                                                                 \
	}                                                                                                                  \
	void name##_ref(size_t handle) { name##_lookup.at(handle).second++; }                                              \
	void name##_unref(size_t handle) {                                                                                 \
		name##_lookup.at(handle).second--;                                                                             \
		if (name##_lookup.at(handle).second == 0) {                                                                    \
			name##_cleanup(handle);                                                                                    \
			name##_recycled.push_back(handle);                                                                         \
			size_t i = name##_lookup.at(handle).first;                                                                 \
			std::swap(name##_dense[i], name##_dense.back());                                                           \
			name##_dense.pop_back();                                                                                   \
			std::swap(name##_reverse[i], name##_reverse.back());                                                       \
			name##_reverse.pop_back();                                                                                 \
			name##_lookup[name##_reverse[i]].first = i;                                                                \
		}                                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
  protected:                                                                                                           \
	T& name##_get(size_t handle) { return name##_dense.at(name##_lookup.at(handle).first); }                           \
                                                                                                                       \
  public:                                                                                                              \
	class T##Handle {                                                                                                  \
	  private:                                                                                                         \
		friend class Self;                                                                                             \
		size_t handle;                                                                                                 \
		Self* render;                                                                                                  \
		T##Handle(size_t handle, Self* render) : handle(handle), render(render) { render->name##_ref(handle); }        \
                                                                                                                       \
	  public:                                                                                                          \
		T##Handle(const T##Handle& src) : T##Handle(src.handle, src.render) {}                                         \
		~T##Handle() { render->name##_unref(handle); }                                                                 \
		T##Handle& operator=(const T##Handle& src) {                                                                   \
			render->name##_unref(handle);                                                                              \
			handle = src.handle;                                                                                       \
			render = src.render;                                                                                       \
			render->name##_ref(handle);                                                                                \
			return *this;                                                                                              \
		}                                                                                                              \
	};                                                                                                                 \
                                                                                                                       \
  protected:                                                                                                           \
	T##Handle name##_insert(const T& item) { return T##Handle(name##_insert_unsafe(item), this); }                     \
	T& name##_get(T##Handle& handle) { return name##_dense.at(name##_lookup.at(handle.handle).first); }

#define INSTANCE_CONTAINER(T, name, Self)                                                                              \
  private:                                                                                                             \
	std::vector<T> name##_dense;                                                                                       \
	std::vector<size_t> name##_lookup;                                                                                 \
	std::vector<size_t> name##_reverse;                                                                                \
	std::vector<size_t> name##_recycled;                                                                               \
	void name##_setup(size_t handle);                                                                                  \
	void name##_cleanup(size_t handle);                                                                                \
	size_t name##_insert_unsafe(const T& item) {                                                                       \
		size_t handle = 0;                                                                                             \
		if (name##_recycled.empty()) {                                                                                 \
			handle = name##_lookup.size();                                                                             \
			name##_lookup.emplace_back();                                                                              \
		} else {                                                                                                       \
			handle = name##_recycled.back();                                                                           \
			name##_recycled.pop_back();                                                                                \
		}                                                                                                              \
		name##_dense.push_back(item);                                                                                  \
		name##_reverse.push_back(handle);                                                                              \
		name##_lookup[handle] = name##_dense.size() - 1;                                                               \
		name##_setup(handle);                                                                                          \
		return handle;                                                                                                 \
	}                                                                                                                  \
	void name##_delete(size_t handle) {                                                                                \
		name##_cleanup(handle);                                                                                        \
		name##_recycled.push_back(handle);                                                                             \
		size_t i = name##_lookup.at(handle);                                                                           \
		std::swap(name##_dense[i], name##_dense.back());                                                               \
		name##_dense.pop_back();                                                                                       \
		std::swap(name##_reverse[i], name##_reverse.back());                                                           \
		name##_reverse.pop_back();                                                                                     \
		name##_lookup[name##_reverse[i]] = i;                                                                          \
	}                                                                                                                  \
	T& name##_get(size_t handle) { return name##_dense.at(name##_lookup.at(handle)); }                                 \
                                                                                                                       \
  public:                                                                                                              \
	class T##Handle {                                                                                                  \
	  private:                                                                                                         \
		friend class Self;                                                                                             \
		size_t handle;                                                                                                 \
		T##Handle(size_t handle) : handle(handle) {}                                                                   \
		T##Handle(const T##Handle& src) = delete;                                                                      \
                                                                                                                       \
	  public:                                                                                                          \
		T##Handle() { handle = std::numeric_limits<size_t>::max(); }                                                   \
		T##Handle(T##Handle&& src) : handle(src.handle) { src.handle = std::numeric_limits<size_t>::max(); }           \
		T##Handle& operator=(T##Handle&& src) {                                                                        \
			handle = src.handle;                                                                                       \
			src.handle = std::numeric_limits<size_t>::max();                                                           \
			return *this;                                                                                              \
		}                                                                                                              \
	};                                                                                                                 \
                                                                                                                       \
  protected:                                                                                                           \
	T##Handle name##_insert(const T& item) { return T##Handle(name##_insert_unsafe(item)); }                           \
	T& name##_get(T##Handle& handle) { return name##_dense.at(name##_lookup.at(handle.handle)); }                      \
	void name##_delete(T##Handle handle) { name##_delete(handle.handle); }

typedef uint TextureHandle;

class Core {
	// GL typecasts
  protected:
	typedef int GLsizei;
	typedef unsigned int GLuint;
	typedef signed long int GLsizeiptr;

	// Begin Resources

  protected:
	struct Mesh {
		GLuint vao;
		GLsizei count;
		std::vector<GLuint> buffers;
	};
	RESOURCE_CONTAINER(Mesh, meshes, Core)

  protected:
	struct Shader {
		GLuint shader;
		enum Type { Opaque = 1 << 0, Depth = 1 << 1, Shadow = 1 << 2, Skybox = 1 << 4 };
		friend inline Type operator|(const Type lhs, const Type rhs) {
			return static_cast<Type>(static_cast<int>(lhs) | static_cast<int>(rhs));
		}
		Type type;
		std::unordered_set<size_t> materials = {};
	};
	RESOURCE_CONTAINER(Shader, shaders, Core)

  protected:
	struct Material {
		struct ShaderConfig {
			ShaderHandle shader;
			GLuint uniform;
			std::vector<TextureHandle> textures;
		};
		std::vector<ShaderConfig> shaders;
		std::unordered_set<size_t> surfaces = {};
	};
	RESOURCE_CONTAINER(Material, materials, Core)

	// End Resources

	// Begin Instances

  protected:
	struct Surface {
		MeshHandle mesh;
		MaterialHandle material;
		mat4 transform;
	};
	INSTANCE_CONTAINER(Surface, surfaces, Core)

  public:
	SurfaceHandle surface_create(MeshHandle& mesh, MaterialHandle& material, mat4 transform = mat4(1.0f)) {
		return surfaces_insert(Surface{.mesh = mesh, .material = material, .transform = transform});
	}

	MeshHandle surface_get_mesh(SurfaceHandle& surface) { return surfaces_get(surface).mesh; }
	void surface_set_mesh(SurfaceHandle& surface, MeshHandle& mesh) { surfaces_get(surface).mesh = mesh; }

	MaterialHandle surface_get_material(SurfaceHandle& surface) { return surfaces_get(surface).material; }
	void surface_set_material(SurfaceHandle& surface, MaterialHandle& material) {
		materials_get(surfaces_get(surface).material).surfaces.erase(surface.handle);
		surfaces_get(surface).material = material;
		materials_get(material).surfaces.emplace(surface.handle);
	}

	mat4 surface_get_transform(SurfaceHandle& surface) { return surfaces_get(surface).transform; }
	void surface_set_transform(SurfaceHandle& surface, mat4 transform) { surfaces_get(surface).transform = transform; }

	void surface_delete(SurfaceHandle surface) { surfaces_delete(std::move(surface)); }

  protected:
	struct DirLight {
		vec3 dir;
		vec3 colour;
	};
	INSTANCE_CONTAINER(DirLight, dir_lights, Core)
  public:
	DirLightHandle dir_light_create(vec3 colour, vec3 dir) {
		DirLightHandle handle = dir_lights_insert(DirLight{});
		dir_light_set_colour(handle, colour);
		dir_light_set_dir(handle, dir);
		return handle;
	}

	void dir_light_set_colour(DirLightHandle& handle, vec3 colour) { dir_lights_get(handle).colour = colour; }
	void dir_light_set_dir(DirLightHandle& handle, vec3 dir) { dir_lights_get(handle).dir = normalize(dir); }
	// End Instances

  protected:
	template <class T> static size_t vector_size(const std::vector<T>& vec) { return sizeof(T) * vec.size(); }

	struct Camera {
		mat4 proj;
		mat4 view;
		vec3 camPos;
	};

	int width, height;

	GLuint cameraBuffer;
	float fov;
	mat4 cameraPos;

	const int lightmapSize = 4096;
	const float lightmapCoverage = 300;

	GLuint dirLightBuffer, dirLightShadow;

	enum class RenderOrder {
		Simple,
		Shader,
	};
	void renderScene(Shader::Type type, RenderOrder order = RenderOrder::Shader);

  public:
	Core(void (*(const char*))());
	Core(const Core&) = delete;

	void resize(int width, int height) {
		this->width = width;
		this->height = height;
	}

	void camera_set_pos(mat4 pos) { cameraPos = glm::inverse(pos); }
	void camera_set_fov(float degrees) { fov = radians(degrees); }

	enum TextureFlags { NONE = 0, SRGB = 1 << 0, MIPMAPPED = 1 << 1, ANIOSTROPIC = 1 << 2, CLAMPED = 1 << 3 };
	friend inline TextureFlags operator|(const TextureFlags lhs, const TextureFlags rhs) {
		return static_cast<TextureFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}
	TextureHandle create_texture(int width, int height, int channels, TextureFlags flags, void* data);

	void run();
};

typedef Core::MeshHandle MeshHandle;
typedef Core::ShaderHandle ShaderHandle;
typedef Core::MaterialHandle MaterialHandle;

typedef Core::SurfaceHandle SurfaceHandle;
typedef Core::DirLightHandle DirLightHandle;

} // namespace Render