#pragma once

#include <glm/gtc/integer.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Render {

using namespace glm;

typedef unsigned int GLuint;
typedef int GLsizei;

struct MeshDef {
	struct Vertex {
		vec3 pos;
		vec3 normal;
		vec2 uv;
	};
	std::vector<Vertex> verticies;
	std::vector<uint32_t> indicies;
};

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

typedef uint MeshHandle;
typedef uint InstanceHandle;
typedef uint MaterialHandle;
typedef uint DirLightHandle;
typedef uint TextureHandle;
typedef uint ShaderHandle;

class Core {
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

	struct Instance {
		uint model;
		uint mat;
		mat4 trans;
	};
	Container<Instance> instances;

	struct Mesh {
		GLuint vao;
		uint count;
		std::vector<GLuint> buffers;
	};
	Container<Mesh> meshes;

	struct ShaderConfig {
		uint shader;
		GLuint uniform;
		std::vector<TextureHandle> textures;
	};
	struct Material {
		std::vector<ShaderConfig> shaders;
		std::unordered_set<InstanceHandle> instances = {};
	};
	Container<Material> materials;
	MaterialHandle register_material(Material mat) {
		auto handle = materials.emplace(mat);
		for (auto& shaderconf : mat.shaders) {
			shaders.at(shaderconf.shader).materials.emplace(handle);
		}
		return handle;
	}

	struct Shader {
		GLuint shader;
		enum Type { Opaque = 1 << 0, Depth = 1 << 1, Shadow = 1 << 2, Skybox = 1 << 4, UI = 1 << 5 };
		friend inline Type operator|(const Type lhs, const Type rhs) {
			return static_cast<Type>(static_cast<int>(lhs) | static_cast<int>(rhs));
		}
		Type type;
		std::unordered_set<MaterialHandle> materials = {};
	};
	Container<Shader> shaders;

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

	MeshHandle create_mesh(MeshDef);
	void delete_mesh(MeshHandle handle);
	InstanceHandle create_instance(MeshHandle mesh, MaterialHandle mat, mat4 trans = mat4(1.0f)) {
		InstanceHandle instance = instances.emplace();
		instance_set_mesh(instance, mesh);
		instance_set_material(instance, mat);
		instance_set_trans(instance, trans);
		return instance;
	}
	void instance_set_mesh(InstanceHandle instance, MeshHandle mesh) { instances.at(instance).model = mesh; }
	void instance_set_material(InstanceHandle instance, MaterialHandle mat) {
		materials.at(instances.at(instance).mat).instances.erase(instance);
		instances.at(instance).mat = mat;
		materials.at(mat).instances.emplace(instance);
	}
	void instance_set_trans(InstanceHandle instance, mat4 trans) { instances.at(instance).trans = trans; }
	void delete_instance(InstanceHandle instance) { instances.erase(instance); }

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

} // namespace Render