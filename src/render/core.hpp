#pragma once

#include <array>
#include <glm/gtc/integer.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
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

template <class T> class Container {
  public:
	typedef uint Handle;

  private:
	std::unordered_map<Handle, T> map;
	Handle next_handle = 0;

  public:
	Handle emplace() {
		Handle handle;
		do
			handle = next_handle++;
		while (map.contains(handle));
		map[handle];
		return handle;
	}
	template <typename... Args> Handle emplace(Args&&... args) {
		Handle handle;
		do
			handle = next_handle++;
		while (map.contains(handle));
		map.emplace(handle, std::forward<Args>(args)...);
		return handle;
	}
	void erase(Handle handle) { map.erase(handle); }
	T& at(const Handle& handle) { return map.at(handle); }
	typename std::unordered_map<Handle, T>::iterator begin() { return map.begin(); }
	typename std::unordered_map<Handle, T>::iterator end() { return map.end(); }
	void vector_push(std::vector<T>& v) {
		v.reserve(v.size() + map.size());
		for (auto& i : map) {
			v.push_back(i.second);
		}
	}
};

typedef uint DirLightHandle;
typedef uint TextureHandle;

class Core {
	// GL typecasts
  protected:
	typedef int GLsizei;
	typedef unsigned int GLuint;
	typedef signed long int GLsizeiptr;

	// Begin Resources

  protected:
	struct Buffer {
		GLuint buffer;
	};
	RESOURCE_CONTAINER(Buffer, buffers, Core)
  protected:
	BufferHandle buffer_create(GLsizeiptr size, const void* data);

  public:
	std::vector<BufferHandle> buffers_create(std::vector<std::vector<uint8_t>>);
	template <class T> BufferHandle buffer_create(std::vector<T> data) {
		return buffer_create(sizeof(T) * data.size(), data.data());
	}

  protected:
	struct Mesh {
		GLuint vao;
		GLsizei count;
		std::vector<BufferHandle> buffers;
		bool indexed = true;
	};
	RESOURCE_CONTAINER(Mesh, meshes, Core)

  public:
	struct MeshDef {
		struct Binding {
			BufferHandle buffer;
			uint64_t offset = 0;
			uint64_t stride;
		};
		std::array<std::optional<Binding>, 16> bindings;
		struct Accessor {
			uint8_t binding;
			bool normalized = false;
			int count; // Per item
			enum class Type { BYTE, UNSIGNED_BYTE, SHORT, UNSIGNED_SHORT, INT, UNSIGNED_INT, FLOAT };
			Type type;
			uint64_t relativeOffset = 0;
		};
		std::array<std::optional<Accessor>, 16> attributes;
		std::optional<BufferHandle> indicies;
		int32_t count;
	};
	MeshHandle mesh_create(MeshDef);

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

	struct DirLight {
		vec3 dir;
		float _pad0;
		vec3 colour;
		float _pad1;
		mat4 shadowMapTrans;
	};
	Container<DirLight> dirLights;
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

	DirLightHandle create_dir_light(vec3 colour, vec3 dir) {
		uint handle = dirLights.emplace();
		dir_light_set_colour(handle, colour);
		dir_light_set_dir(handle, dir);
		return handle;
	}
	void dir_light_set_colour(DirLightHandle handle, vec3 colour) { dirLights.at(handle).colour = colour; }
	void dir_light_set_dir(DirLightHandle handle, vec3 dir) {
		dirLights.at(handle).dir = normalize(dir);
		dirLights.at(handle).shadowMapTrans = ortho(
												  -lightmapCoverage, lightmapCoverage, -lightmapCoverage,
												  lightmapCoverage, -lightmapCoverage, lightmapCoverage) *
			lookAt(vec3{0, 0, 0}, -dirLights.at(handle).dir, vec3{0, 1, 0});
	}

	enum TextureFlags { NONE = 0, SRGB = 1 << 0, MIPMAPPED = 1 << 1, ANIOSTROPIC = 1 << 2, CLAMPED = 1 << 3 };
	friend inline TextureFlags operator|(const TextureFlags lhs, const TextureFlags rhs) {
		return static_cast<TextureFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}
	TextureHandle create_texture(int width, int height, int channels, TextureFlags flags, void* data);

	void run();
};

typedef Core::BufferHandle BufferHandle;
typedef Core::MeshHandle MeshHandle;
typedef Core::ShaderHandle ShaderHandle;
typedef Core::MaterialHandle MaterialHandle;

typedef Core::SurfaceHandle SurfaceHandle;

} // namespace Render