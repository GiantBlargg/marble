#include "gltf.hpp"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;

namespace Render {

#define FROM_JSON(member) j.at(#member).get_to(t.member);
#define FROM_JSON_OPTIONAL(member)                                                                                     \
	if (j.contains(#member))                                                                                           \
	FROM_JSON(member)
#define FROM_JSON_OPTIONAL_TYPE(Type, member)                                                                          \
	if (j.contains(#member)) {                                                                                         \
		Type member;                                                                                                   \
		j.at(#member).get_to(member);                                                                                  \
		t.member = member;                                                                                             \
	}
#define JSON_ENUM(ENUM_TYPE, ...)                                                                                      \
	template <typename BasicJsonType> friend inline void from_json(const BasicJsonType& j, ENUM_TYPE& e) {             \
		static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");                                 \
		static const std::pair<ENUM_TYPE, BasicJsonType> m[] = __VA_ARGS__;                                            \
		auto it = std::find_if(                                                                                        \
			std::begin(m), std::end(m),                                                                                \
			[&j](const std::pair<ENUM_TYPE, BasicJsonType>& ej_pair) -> bool { return ej_pair.second == j; });         \
		e = ((it != std::end(m)) ? it : std::begin(m))->first;                                                         \
	}

struct Gltf {
	std::vector<std::string> extensionsUsed;
	std::vector<std::string> extensionsRequired;
	enum class ComponentType { BYTE, UNSIGNED_BYTE, SHORT, UNSIGNED_SHORT, INT, UNSIGNED_INT, FLOAT };
	JSON_ENUM(
		ComponentType,
		{{ComponentType::BYTE, 5120},
		 {ComponentType::UNSIGNED_BYTE, 5121},
		 {ComponentType::SHORT, 5122},
		 {ComponentType::UNSIGNED_SHORT, 5123},
		 {ComponentType::INT, 5124},
		 {ComponentType::UNSIGNED_INT, 5125},
		 {ComponentType::FLOAT, 5126}})
	struct Accessor {
		std::optional<uint64_t> bufferView;
		uint64_t byteOffset = 0;
		ComponentType componentType;
		bool normalized = false;
		uint64_t count;
		enum class Type { SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4 };
		JSON_ENUM(
			Type,
			{{Type::SCALAR, "SCALAR"},
			 {Type::VEC2, "VEC2"},
			 {Type::VEC3, "VEC3"},
			 {Type::VEC4, "VEC4"},
			 {Type::MAT2, "MAT2"},
			 {Type::MAT3, "MAT3"},
			 {Type::MAT4, "MAT4"}})
		Type type;
		std::vector<double> max;
		std::vector<double> min;
		struct Sparse {
			uint64_t count;
			struct Indices {
				uint64_t bufferView;
				uint64_t byteOffset = 0;
				ComponentType componentType;

				friend void from_json(const json& j, Indices& t) {
					FROM_JSON(bufferView)
					FROM_JSON_OPTIONAL(byteOffset)
					FROM_JSON(componentType)
				}
			};
			Indices indices;
			struct Values {
				uint64_t bufferView;
				uint64_t byteOffset = 0;

				friend void from_json(const json& j, Values& t) {
					FROM_JSON(bufferView)
					FROM_JSON_OPTIONAL(byteOffset)
				}
			};
			Values values;

			friend void from_json(const json& j, Sparse& t) {
				FROM_JSON(count)
				FROM_JSON(indices)
				FROM_JSON(values)
			}
		};
		std::optional<Sparse> sparse;
		std::optional<std::string> name;

		friend void from_json(const json& j, Accessor& t) {
			FROM_JSON_OPTIONAL_TYPE(uint64_t, bufferView)
			FROM_JSON_OPTIONAL(byteOffset)
			FROM_JSON(componentType)
			FROM_JSON_OPTIONAL(normalized)
			FROM_JSON(count)
			FROM_JSON(type)
			FROM_JSON_OPTIONAL(max)
			FROM_JSON_OPTIONAL(min)
			FROM_JSON_OPTIONAL_TYPE(Sparse, sparse)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Accessor> accessors;
	struct Animation {
		struct Channel {
			uint64_t sampler;
			struct Target {
				std::optional<uint64_t> node;
				enum class Path { translation, rotation, scale, weights };
				JSON_ENUM(
					Path,
					{{Path::translation, "translation"},
					 {Path::rotation, "rotation"},
					 {Path::scale, "scale"},
					 {Path::weights, "weights"}})
				Path path;

				friend void from_json(const json& j, Target& t) {
					FROM_JSON_OPTIONAL_TYPE(uint64_t, node)
					FROM_JSON(path)
				}
			};
			Target target;

			friend void from_json(const json& j, Channel& t) {
				FROM_JSON(sampler)
				FROM_JSON(target)
			}
		};
		std::vector<Channel> channels;
		struct Sampler {
			uint64_t input;
			enum class Interpolation {
				LINEAR,
				STEP,
				CUBICSPLINE,
			};
			JSON_ENUM(
				Interpolation,
				{{Interpolation::LINEAR, "LINEAR"},
				 {Interpolation::STEP, "STEP"},
				 {Interpolation::CUBICSPLINE, "CUBICSPLINE"}})
			Interpolation interpolation = Interpolation::LINEAR;
			uint64_t output;

			friend void from_json(const json& j, Sampler& t) {
				FROM_JSON(input)
				FROM_JSON_OPTIONAL(interpolation)
				FROM_JSON(output)
			}
		};
		std::vector<Sampler> samplers;
		std::optional<std::string> name;

		friend void from_json(const json& j, Animation& t) {
			FROM_JSON(channels)
			FROM_JSON(samplers)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Animation> animations;
	struct Asset {
		std::optional<std::string> copyright;
		std::optional<std::string> generator;
		std::string version;
		std::optional<std::string> minVersion;

		friend void from_json(const json& j, Asset& t) {
			FROM_JSON_OPTIONAL_TYPE(std::string, copyright)
			FROM_JSON_OPTIONAL_TYPE(std::string, generator)
			FROM_JSON(version)
			FROM_JSON_OPTIONAL_TYPE(std::string, minVersion)
		}
	};
	Asset asset;
	struct Buffer {
		std::optional<std::string> uri;
		uint64_t byteLength;
		std::optional<std::string> name;

		friend void from_json(const json& j, Buffer& t) {
			FROM_JSON_OPTIONAL_TYPE(std::string, uri)
			FROM_JSON(byteLength)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Buffer> buffers;
	struct BufferView {
		uint64_t buffer;
		uint64_t byteOffset = 0;
		uint64_t byteLength;
		std::optional<uint64_t> byteStride;
		enum class Target { ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER };
		JSON_ENUM(Target, {{Target::ARRAY_BUFFER, 34962}, {Target::ELEMENT_ARRAY_BUFFER, 34963}})
		Target target;
		std::optional<std::string> name;

		friend void from_json(const json& j, BufferView& t) {
			FROM_JSON(buffer)
			FROM_JSON_OPTIONAL(byteOffset)
			FROM_JSON(byteLength)
			FROM_JSON_OPTIONAL_TYPE(uint64_t, byteStride)
			FROM_JSON_OPTIONAL(target)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<BufferView> bufferViews;
	struct Camera {
		struct Orthographic {
			double xmag;
			double ymag;
			double zfar;
			double znear;

			friend void from_json(const json& j, Orthographic& t) {
				FROM_JSON(xmag)
				FROM_JSON(ymag)
				FROM_JSON(zfar)
				FROM_JSON(znear)
			}
		};
		std::optional<Orthographic> orthographic;
		struct Perspective {
			std::optional<double> aspectRatio;
			double yfov;
			std::optional<double> zfar;
			double znear;

			friend void from_json(const json& j, Perspective& t) {
				FROM_JSON_OPTIONAL_TYPE(double, aspectRatio)
				FROM_JSON(yfov)
				FROM_JSON_OPTIONAL_TYPE(double, zfar)
				FROM_JSON(znear)
			}
		};
		std::optional<Perspective> perspective;
		enum class Type { perspective, orthographic };
		JSON_ENUM(Type, {{Type::perspective, "perspective"}, {Type::orthographic, "orthographic"}})
		Type type;
		std::optional<std::string> name;

		friend void from_json(const json& j, Camera& t) {
			FROM_JSON_OPTIONAL_TYPE(Orthographic, orthographic)
			FROM_JSON_OPTIONAL_TYPE(Perspective, perspective)
			FROM_JSON(type)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Camera> cameras;
	struct Image {
		std::optional<std::string> uri;
		std::optional<std::string> mimeType;
		std::optional<uint64_t> bufferView;
		std::optional<std::string> name;

		friend void from_json(const json& j, Image& t) {
			FROM_JSON_OPTIONAL_TYPE(std::string, uri)
			FROM_JSON_OPTIONAL_TYPE(std::string, mimeType)
			FROM_JSON_OPTIONAL_TYPE(uint64_t, bufferView)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Image> images;
	struct Material {
		struct TextureInfo {
			uint64_t index;
			uint64_t texCoord = 0;

			friend void from_json(const json& j, TextureInfo& t) {
				FROM_JSON(index)
				FROM_JSON_OPTIONAL(texCoord)
			}
		};
		struct PbrMetallicRoughness {
			std::array<double, 4> baseColorFactor = {1, 1, 1, 1};
			std::optional<TextureInfo> baseColorTexture;
			double metallicFactor = 1;
			double roughnessFactor = 1;
			std::optional<TextureInfo> metallicRoughnessTexture;

			friend void from_json(const json& j, PbrMetallicRoughness& t) {
				FROM_JSON_OPTIONAL(baseColorFactor)
				FROM_JSON_OPTIONAL_TYPE(TextureInfo, baseColorTexture)
				FROM_JSON_OPTIONAL(metallicFactor)
				FROM_JSON_OPTIONAL(roughnessFactor)
				FROM_JSON_OPTIONAL_TYPE(TextureInfo, metallicRoughnessTexture)
			}
		};
		std::optional<PbrMetallicRoughness> pbrMetallicRoughness;
		struct NormalTextureInfo {
			uint64_t index;
			uint64_t texCoord = 0;
			double scale = 1;

			friend void from_json(const json& j, NormalTextureInfo& t) {
				FROM_JSON(index)
				FROM_JSON_OPTIONAL(texCoord)
				FROM_JSON_OPTIONAL(scale)
			}
		};
		std::optional<NormalTextureInfo> normalTexture;
		struct OcclusionTextureInfo {
			uint64_t index;
			uint64_t texCoord = 0;
			double strength = 1;

			friend void from_json(const json& j, OcclusionTextureInfo& t) {
				FROM_JSON(index)
				FROM_JSON_OPTIONAL(texCoord)
				FROM_JSON_OPTIONAL(strength)
			}
		};
		std::optional<OcclusionTextureInfo> occlusionTexture;
		std::optional<TextureInfo> emissiveTexture;
		std::array<std::string, 3> emissiveFactor = {0, 0, 0};
		enum class AlphaMode {
			OPAQUE,
			MASK,
			BLEND,
		};
		JSON_ENUM(AlphaMode, {{AlphaMode::OPAQUE, "OPAQUE"}, {AlphaMode::MASK, "MASK"}, {AlphaMode::BLEND, "BLEND"}})
		AlphaMode alphaMode = AlphaMode::OPAQUE;
		double alphaCutoff = 0.5;
		bool doubleSided = false;
		std::optional<std::string> name;

		friend void from_json(const json& j, Material& t) {
			FROM_JSON_OPTIONAL_TYPE(PbrMetallicRoughness, pbrMetallicRoughness)
			FROM_JSON_OPTIONAL_TYPE(NormalTextureInfo, normalTexture)
			FROM_JSON_OPTIONAL_TYPE(OcclusionTextureInfo, occlusionTexture)
			FROM_JSON_OPTIONAL_TYPE(TextureInfo, emissiveTexture)
			FROM_JSON_OPTIONAL(emissiveFactor)
			FROM_JSON_OPTIONAL(alphaMode)
			FROM_JSON_OPTIONAL(alphaCutoff)
			FROM_JSON_OPTIONAL(doubleSided)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Material> materials;
	struct Mesh {
		struct Primitive {
			struct Attributes {
				std::optional<uint64_t> position;
				std::optional<uint64_t> normal;
				std::optional<uint64_t> tangent;
				std::vector<uint64_t> texcoord;
				std::vector<uint64_t> color;
				std::vector<uint64_t> joints;
				std::vector<uint64_t> weights;

				friend void from_json(const json& j, Attributes& attributes) {
					for (auto& [key, val] : j.items()) {
						uint64_t value;
						val.get_to(value);
						if (key == "POSITION")
							attributes.position = value;
						else if (key == "NORMAL")
							attributes.normal = value;
						else if (key == "TAGENT")
							attributes.tangent = value;
						else
							std::cout << "Unknown Attribute: " << key << std::endl;
					}
				}
			};
			Attributes attributes;
			std::optional<uint64_t> indices;
			std::optional<uint64_t> material;
			enum class Mode { POINTS, LINES, LINE_LOOP, LINE_STRIP, TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN };
			Mode mode = Mode::TRIANGLES;
			struct Target {
				std::optional<uint64_t> POSITION;
				std::optional<uint64_t> NORMAL;
				std::optional<uint64_t> TANGENT;

				friend void from_json(const json& j, Target& t) {
					FROM_JSON_OPTIONAL_TYPE(uint64_t, POSITION)
					FROM_JSON_OPTIONAL_TYPE(uint64_t, NORMAL)
					FROM_JSON_OPTIONAL_TYPE(uint64_t, TANGENT)
				}
			};
			std::vector<Target> targets;

			friend void from_json(const json& j, Primitive& t) {
				FROM_JSON(attributes)
				FROM_JSON_OPTIONAL_TYPE(uint64_t, indices)
				FROM_JSON_OPTIONAL_TYPE(uint64_t, material)
				FROM_JSON_OPTIONAL(mode)
				FROM_JSON_OPTIONAL(targets)
			}
		};
		std::vector<Primitive> primitives;
		std::vector<double> weights;
		std::optional<std::string> name;

		friend void from_json(const json& j, Mesh& t) {
			FROM_JSON(primitives)
			FROM_JSON_OPTIONAL(weights)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Mesh> meshes;
	struct Node {
		std::optional<uint64_t> camera;
		std::vector<uint64_t> children;
		std::optional<uint64_t> skin;
		std::array<double, 16> matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
		std::optional<uint64_t> mesh;
		std::array<double, 4> rotation = {0, 0, 0, 1};
		std::array<double, 3> scale = {1, 1, 1};
		std::array<double, 3> translation = {0, 0, 0};
		std::vector<double> weights;
		std::optional<std::string> name;

		friend void from_json(const json& j, Node& t) {
			FROM_JSON_OPTIONAL_TYPE(uint64_t, camera)
			FROM_JSON_OPTIONAL(children)
			FROM_JSON_OPTIONAL_TYPE(uint64_t, skin)
			FROM_JSON_OPTIONAL(matrix)
			FROM_JSON_OPTIONAL_TYPE(uint64_t, mesh)
			FROM_JSON_OPTIONAL(rotation)
			FROM_JSON_OPTIONAL(scale)
			FROM_JSON_OPTIONAL(translation)
			FROM_JSON_OPTIONAL(weights)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Node> nodes;
	struct Sampler {
		enum class Filter {
			NEAREST,
			LINEAR,
			NEAREST_MIPMAP_NEAREST,
			LINEAR_MIPMAP_NEAREST,
			NEAREST_MIPMAP_LINEAR,
			LINEAR_MIPMAP_LINEAR
		};
		JSON_ENUM(
			Filter,
			{{Filter::NEAREST, 9728},
			 {Filter::LINEAR, 9729},
			 {Filter::NEAREST_MIPMAP_NEAREST, 9984},
			 {Filter::LINEAR_MIPMAP_NEAREST, 9985},
			 {Filter::NEAREST_MIPMAP_LINEAR, 9986},
			 {Filter::LINEAR_MIPMAP_LINEAR, 9987}})
		std::optional<Filter> magFilter;
		std::optional<Filter> minFilter;
		enum class Wrap { CLAMP_TO_EDGE, MIRRORED_REPEAT, REPEAT };
		JSON_ENUM(Wrap, {{Wrap::CLAMP_TO_EDGE, 33071}, {Wrap::MIRRORED_REPEAT, 33648}, {Wrap::REPEAT, 10497}})
		Wrap wrapS = Wrap::REPEAT;
		Wrap wrapT = Wrap::REPEAT;
		std::optional<std::string> name;

		friend void from_json(const json& j, Sampler& t) {
			FROM_JSON_OPTIONAL_TYPE(Filter, magFilter)
			FROM_JSON_OPTIONAL_TYPE(Filter, minFilter)
			FROM_JSON_OPTIONAL(wrapS)
			FROM_JSON_OPTIONAL(wrapT)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Sampler> samplers;
	std::optional<uint64_t> scene;
	struct Scene {
		std::vector<uint64_t> nodes;
		std::optional<std::string> name;

		friend void from_json(const json& j, Scene& t) {
			FROM_JSON_OPTIONAL(nodes)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Scene> scenes;
	struct Skin {
		std::optional<uint64_t> inverseBindMatrices;
		std::optional<uint64_t> skeleton;
		std::vector<uint64_t> joints;
		std::optional<std::string> name;

		friend void from_json(const json& j, Skin& t) {
			FROM_JSON_OPTIONAL_TYPE(uint64_t, inverseBindMatrices)
			FROM_JSON_OPTIONAL_TYPE(uint64_t, skeleton)
			FROM_JSON(joints)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Skin> skins;
	struct Texture {
		std::optional<uint64_t> sampler;
		std::optional<uint64_t> source;
		std::optional<std::string> name;

		friend void from_json(const json& j, Texture& t) {
			FROM_JSON_OPTIONAL_TYPE(uint64_t, sampler)
			FROM_JSON_OPTIONAL_TYPE(uint64_t, source)
			FROM_JSON_OPTIONAL_TYPE(std::string, name)
		}
	};
	std::vector<Texture> textures;

	friend void from_json(const json& j, Gltf& t) {
		FROM_JSON_OPTIONAL(extensionsUsed)
		FROM_JSON_OPTIONAL(extensionsRequired)
		FROM_JSON_OPTIONAL(accessors)
		FROM_JSON_OPTIONAL(animations)
		FROM_JSON(asset)
		FROM_JSON_OPTIONAL(buffers)
		FROM_JSON_OPTIONAL(cameras)
		FROM_JSON_OPTIONAL(images)
		FROM_JSON_OPTIONAL(materials)
		FROM_JSON_OPTIONAL(bufferViews)
		FROM_JSON_OPTIONAL(meshes)
		FROM_JSON_OPTIONAL(nodes)
		FROM_JSON_OPTIONAL(samplers)
		FROM_JSON_OPTIONAL_TYPE(uint64_t, scene)
		FROM_JSON_OPTIONAL(scenes)
		FROM_JSON_OPTIONAL(skins)
		FROM_JSON_OPTIONAL(textures)
	}
};

Render::StandardMesh::Accessor
convert_accessor(const Gltf& gltf, const std::vector<BufferHandle>& buffer_handles, const uint64_t accessor_index) {
	static const std::unordered_map<Gltf::Accessor::Type, int> element_size = {
		{Gltf::Accessor::Type::SCALAR, 1}, {Gltf::Accessor::Type::VEC2, 2}, {Gltf::Accessor::Type::VEC3, 3},
		{Gltf::Accessor::Type::VEC4, 4},   {Gltf::Accessor::Type::MAT2, 4}, {Gltf::Accessor::Type::MAT3, 9},
		{Gltf::Accessor::Type::MAT4, 16}};
	static const std::unordered_map<Gltf::ComponentType, int> type_size = {
		{Gltf::ComponentType::BYTE, 1},  {Gltf::ComponentType::UNSIGNED_BYTE, 1},
		{Gltf::ComponentType::SHORT, 2}, {Gltf::ComponentType::UNSIGNED_SHORT, 2},
		{Gltf::ComponentType::INT, 4},   {Gltf::ComponentType::UNSIGNED_INT, 4},
		{Gltf::ComponentType::FLOAT, 4}};

	const Gltf::Accessor& accessor = gltf.accessors[accessor_index];
	const Gltf::BufferView& buffer_view = gltf.bufferViews[accessor.bufferView.value()];
	const BufferHandle& buffer = buffer_handles[buffer_view.buffer];
	const uint64_t stride =
		buffer_view.byteStride.value_or(element_size.at(accessor.type) * type_size.at(accessor.componentType));

	return Render::StandardMesh::Accessor{
		.buffer = buffer,
		.bufferOffset = buffer_view.byteOffset,
		.stride = stride,
		.relativeOffset = accessor.byteOffset};
}

Model load_gltf(std::filesystem::path path, Render& render) {
	std::ifstream gltf_file(path);
	json j = json::parse(gltf_file);
	Gltf gltf = j.get<Gltf>();

	std::vector<std::vector<uint8_t>> buffers_data;
	buffers_data.resize(gltf.buffers.size());
	for (size_t i = 0; i < buffers_data.size(); i++) {
		buffers_data[i].resize(gltf.buffers[i].byteLength);
		if (!gltf.buffers[i].uri.has_value()) {
			std::cout << "I don't know how to load buffer " << i << std::endl;
			continue;
		}
		auto dir = path.parent_path().append(gltf.buffers[i].uri.value());
		std::ifstream buf(dir, std::ifstream::in | std::ifstream::binary);
		buf.read(reinterpret_cast<char*>(buffers_data[i].data()), buffers_data[i].size());
	}

	std::vector<BufferHandle> buffer_handles = render.buffers_create(buffers_data);

	MaterialHandle default_material = render.create_pbr_material(MaterialPBR{.albedoFactor = vec4(1.0f)});

	std::vector<std::vector<Model::Surface>> models;
	models.resize(gltf.meshes.size());
	for (size_t i = 0; i < models.size(); i++) {
		models[i].reserve(gltf.meshes[i].primitives.size());
		for (auto& prim : gltf.meshes[i].primitives) {
			if (!prim.attributes.position.has_value())
				continue;
			int count = static_cast<int>(gltf.accessors[prim.indices.value_or(prim.attributes.position.value())].count);
			Render::StandardMesh mesh{
				.position = convert_accessor(gltf, buffer_handles, prim.attributes.position.value()), .count = count};
			if (prim.attributes.normal.has_value())
				mesh.normal = convert_accessor(gltf, buffer_handles, prim.attributes.normal.value());
			if (prim.attributes.tangent.has_value())
				mesh.tangent = convert_accessor(gltf, buffer_handles, prim.attributes.tangent.value());
			mesh.texcoord.reserve(prim.attributes.texcoord.size());
			for (auto accessor : prim.attributes.texcoord)
				mesh.texcoord.push_back(convert_accessor(gltf, buffer_handles, accessor));
			for (auto accessor : prim.attributes.color)
				mesh.color.push_back(convert_accessor(gltf, buffer_handles, accessor));

			models[i].push_back(
				Model::Surface{.mesh = render.standard_mesh_create(mesh), .material = default_material});
		}
	}

	return Model{.render = render, .surfaces = models[0]};
}

} // namespace Render