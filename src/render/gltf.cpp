#include "gltf.hpp"

#include <fstream>
#include <gl.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/packing.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <stb_image.h>

using json = nlohmann::json;

namespace glm {
template <length_t L, typename T> void from_json(const json& j, vec<L, T, defaultp>& v) {
	auto array = j.get<std::array<T, L>>();
	v = *reinterpret_cast<vec<L, T, defaultp>*>(&array);
}

template <typename T> void from_json(const json& j, qua<T, defaultp>& v) {
	auto array = j.get<std::array<T, 4>>();
	v = *reinterpret_cast<qua<T, defaultp>*>(&array);
}

template <length_t C, length_t R, typename T> void from_json(const json& j, mat<C, R, T, defaultp>& m) {
	auto array = j.get<std::array<T, C * R>>();
	m = *reinterpret_cast<mat<C, R, T, defaultp>*>(&array);
}
} // namespace glm

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
			dvec4 baseColorFactor = {1, 1, 1, 1};
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
		PbrMetallicRoughness pbrMetallicRoughness;
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
		dvec3 emissiveFactor = {0, 0, 0};
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
			FROM_JSON_OPTIONAL(pbrMetallicRoughness)
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
					if (j.contains("POSITION"))
						attributes.position = j.at("POSITION").get<uint64_t>();

					if (j.contains("NORMAL"))
						attributes.normal = j.at("NORMAL").get<uint64_t>();

					if (j.contains("TAGENT"))
						attributes.tangent = j.at("TAGENT").get<uint64_t>();

					for (int i = 0; true; i++) {
						std::string name = "TEXCOORD_" + std::to_string(i);
						if (!j.contains(name))
							break;
						attributes.texcoord.push_back(j.at(name).get<uint64_t>());
					}

					for (int i = 0; true; i++) {
						std::string name = "COLOR_" + std::to_string(i);
						if (!j.contains(name))
							break;
						attributes.color.push_back(j.at(name).get<uint64_t>());
					}

					for (int i = 0; true; i++) {
						std::string name = "JOINTS_" + std::to_string(i);
						if (!j.contains(name))
							break;
						attributes.joints.push_back(j.at(name).get<uint64_t>());
					}

					for (int i = 0; true; i++) {
						std::string name = "WEIGHTS_" + std::to_string(i);
						if (!j.contains(name))
							break;
						attributes.weights.push_back(j.at(name).get<uint64_t>());
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
		dmat4 matrix = dmat4(1.0f);
		std::optional<uint64_t> mesh;
		dquat rotation = {1, 0, 0, 0};
		dvec3 scale = {1, 1, 1};
		dvec3 translation = {0, 0, 0};
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
		Filter magFilter = Filter::LINEAR;               // default is application defined
		Filter minFilter = Filter::LINEAR_MIPMAP_LINEAR; // default is application defined
		enum class Wrap { CLAMP_TO_EDGE, MIRRORED_REPEAT, REPEAT };
		JSON_ENUM(Wrap, {{Wrap::CLAMP_TO_EDGE, 33071}, {Wrap::MIRRORED_REPEAT, 33648}, {Wrap::REPEAT, 10497}})
		Wrap wrapS = Wrap::REPEAT;
		Wrap wrapT = Wrap::REPEAT;
		std::optional<std::string> name;

		friend void from_json(const json& j, Sampler& t) {
			FROM_JSON_OPTIONAL(magFilter)
			FROM_JSON_OPTIONAL(minFilter)
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

void accessor_for_each(
	const Gltf& gltf, const std::vector<std::vector<uint8_t>> buffers_data, size_t accessor_index,
	std::function<void(size_t index, const void* data)> functor) {
	const Gltf::Accessor& accessor = gltf.accessors[accessor_index];
	const Gltf::BufferView& bufferView = gltf.bufferViews[accessor.bufferView.value()];
	const std::vector<uint8_t>& buffer = buffers_data.at(bufferView.buffer);

	static const std::unordered_map<Gltf::Accessor::Type, int> element_size = {
		{Gltf::Accessor::Type::SCALAR, 1}, {Gltf::Accessor::Type::VEC2, 2}, {Gltf::Accessor::Type::VEC3, 3},
		{Gltf::Accessor::Type::VEC4, 4},   {Gltf::Accessor::Type::MAT2, 4}, {Gltf::Accessor::Type::MAT3, 9},
		{Gltf::Accessor::Type::MAT4, 16}};
	static const std::unordered_map<Gltf::ComponentType, int> type_size = {
		{Gltf::ComponentType::BYTE, 1},  {Gltf::ComponentType::UNSIGNED_BYTE, 1},
		{Gltf::ComponentType::SHORT, 2}, {Gltf::ComponentType::UNSIGNED_SHORT, 2},
		{Gltf::ComponentType::INT, 4},   {Gltf::ComponentType::UNSIGNED_INT, 4},
		{Gltf::ComponentType::FLOAT, 4}};
	const uint64_t stride =
		bufferView.byteStride.value_or(element_size.at(accessor.type) * type_size.at(accessor.componentType));

	for (size_t i = 0; i < accessor.count; i++) {
		size_t offset = bufferView.byteOffset + stride * i + accessor.byteOffset;
		const uint8_t* ptr = buffer.data() + offset;
		functor(i, ptr);
	}
}

uint8_t base64_value(char c) {
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if (c >= '0' && c <= '9')
		return c - '0' + 52;
	if (c == '+' || c == '-')
		return 62;
	if (c == '/' || c == '_')
		return 63;
	return 0;
}

void base64_decode(std::string base64, std::vector<uint8_t>& buffer) {
	if (buffer.empty()) {
		while (base64.back() == '=')
			base64.pop_back();

		buffer.resize(base64.size() * (3.0f / 4.0f));
	}

	for (size_t i = 0; i < buffer.size(); i++) {
		uint8_t& byte = buffer[i];
		size_t pos_in_group = i % 3;
		size_t string_group_pos = i / 3 * 4;
		switch (pos_in_group) {
		case 0:
			byte =
				(base64_value(base64[string_group_pos + 0]) << 2) + (base64_value(base64[string_group_pos + 1]) >> 4);
			break;
		case 1:
			byte =
				(base64_value(base64[string_group_pos + 1]) << 4) + (base64_value(base64[string_group_pos + 2]) >> 2);
			break;
		case 2:
			byte =
				(base64_value(base64[string_group_pos + 2]) << 6) + (base64_value(base64[string_group_pos + 3]) >> 0);
			break;

		default:
			assert(false);
		}
	}
}

void load_uri(std::string uri, std::filesystem::path current_path, std::vector<uint8_t>& buffer) {
	if (uri.starts_with("data:")) {
		size_t d = uri.find(";base64,");
		if (d != std::string::npos) {
			std::string base64 = uri.substr(d + 8);
			base64_decode(base64, buffer);
		} else {
			std::cout << "I don't know how to load this uri: " << uri << std::endl;
		}
	} else {
		std::filesystem::path dir = current_path.parent_path().append(uri);
		std::ifstream buf(dir, std::ifstream::in | std::ifstream::binary);
		if (buffer.empty()) {
			buf.seekg(0, std::ifstream::end);
			buffer.resize(buf.tellg());
			buf.seekg(0, std::ifstream::beg);
		}
		buf.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
	}
}

dmat4 convert_transform(const Gltf::Node& node) {
	const dmat4 T = translate(dmat4(1.0f), node.translation);
	const dmat4 R = mat4_cast(node.rotation);
	const dmat4 S = scale(dmat4(1.0f), node.scale);

	return T * R * S * node.matrix;
}

void crawl_nodes(
	const Gltf& gltf, const std::vector<uint64>& nodes, std::vector<Model::Surface>& surfaces,
	const std::vector<std::vector<Model::Surface>>& models, const dmat4& parent_transform = mat4(1.0)) {
	for (uint64 n : nodes) {
		auto& node = gltf.nodes[n];
		const dmat4 transform = parent_transform * convert_transform(node);

		if (node.mesh.has_value()) {
			for (auto mesh : models[node.mesh.value()]) {
				mesh.transform = transform;
				surfaces.push_back(mesh);
			}
		}

		crawl_nodes(gltf, node.children, surfaces, models, transform);
	}
}

struct ImageData {
	int width;
	int height;
	int channels;
	std::vector<uint8> data;
};

TextureHandle create_texture(const Gltf& gltf, const std::vector<ImageData>& images, uint64 index, bool srgb = false) {
	const auto& texture_desc = gltf.textures[index];
	const auto& image = images[texture_desc.source.value()];
	Gltf::Sampler sampler;
	if (texture_desc.sampler.has_value()) {
		sampler = gltf.samplers[texture_desc.sampler.value()];
	}

	GLenum format, internalformat;
	switch (image.channels) {
	case 1:
		format = GL_RED;
		internalformat = GL_R8;
		break;
	case 2:
		format = GL_RG;
		internalformat = GL_RG8;
		break;
	case 3:
		format = GL_RGB;
		internalformat = srgb ? GL_SRGB8 : GL_RGB8;
		break;
	case 4:
		format = GL_RGBA;
		internalformat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		break;
	}

	bool mipmaps =
		sampler.minFilter != Gltf::Sampler::Filter::NEAREST && sampler.minFilter != Gltf::Sampler::Filter::LINEAR;

	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	auto levels = 1;
	if (mipmaps)
		levels = glm::max(glm::log2(min(image.width, image.height)), 1);

	glTextureStorage2D(texture, levels, internalformat, image.width, image.height);

	if (sampler.minFilter == Gltf::Sampler::Filter::LINEAR_MIPMAP_LINEAR)
		glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, INFINITY);

	glTextureSubImage2D(texture, 0, 0, 0, image.width, image.height, format, GL_UNSIGNED_BYTE, image.data.data());

	const static std::unordered_map<Gltf::Sampler::Filter, GLint> texture_filter = {
		{Gltf::Sampler::Filter::NEAREST, GL_NEAREST},
		{Gltf::Sampler::Filter::LINEAR, GL_LINEAR},
		{Gltf::Sampler::Filter::NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_NEAREST},
		{Gltf::Sampler::Filter::LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST},
		{Gltf::Sampler::Filter::NEAREST_MIPMAP_LINEAR, GL_NEAREST_MIPMAP_LINEAR},
		{Gltf::Sampler::Filter::LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR}};
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, texture_filter.at(sampler.magFilter));
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, texture_filter.at(sampler.minFilter));

	const static std::unordered_map<Gltf::Sampler::Wrap, GLint> texture_wrap = {
		{Gltf::Sampler::Wrap::CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE},
		{Gltf::Sampler::Wrap::MIRRORED_REPEAT, GL_MIRRORED_REPEAT},
		{Gltf::Sampler::Wrap::REPEAT, GL_REPEAT}};
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, texture_wrap.at(sampler.wrapS));
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, texture_wrap.at(sampler.wrapT));

	if (mipmaps)
		glGenerateTextureMipmap(texture);
	return texture;
}

MaterialPBR convert_material(const Gltf& gltf, const std::vector<ImageData>& images, const Gltf::Material& material) {
	std::optional<TextureHandle> albedoTexture;
	if (material.pbrMetallicRoughness.baseColorTexture.has_value()) {
		albedoTexture =
			create_texture(gltf, images, material.pbrMetallicRoughness.baseColorTexture.value().index, true);
		assert(material.pbrMetallicRoughness.baseColorTexture.value().texCoord == 0);
	}
	std::optional<TextureHandle> metalRoughTexture;
	if (material.pbrMetallicRoughness.metallicRoughnessTexture.has_value()) {
		metalRoughTexture =
			create_texture(gltf, images, material.pbrMetallicRoughness.metallicRoughnessTexture.value().index);
		assert(material.pbrMetallicRoughness.metallicRoughnessTexture.value().texCoord == 0);
	}
	std::optional<TextureHandle> normalTexture;
	if (material.normalTexture.has_value()) {
		normalTexture = create_texture(gltf, images, material.normalTexture.value().index);
		assert(material.normalTexture.value().texCoord == 0);
		assert(material.normalTexture.value().scale == 1);
	}
	std::optional<TextureHandle> occlusionTexture;
	if (material.occlusionTexture.has_value()) {
		occlusionTexture = create_texture(gltf, images, material.occlusionTexture.value().index);
		assert(material.occlusionTexture.value().texCoord == 0);
		assert(material.occlusionTexture.value().strength == 1);
	}
	std::optional<TextureHandle> emissiveTexture;
	if (material.emissiveTexture.has_value()) {
		emissiveTexture = create_texture(gltf, images, material.emissiveTexture.value().index);
		assert(material.emissiveTexture.value().texCoord == 0);
	}

	return MaterialPBR{
		.albedoFactor = material.pbrMetallicRoughness.baseColorFactor,
		.albedoTexture = albedoTexture,
		.metalFactor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor),
		.roughFactor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor),
		.metalRoughTexture = metalRoughTexture,
		.normalTexture = normalTexture,
		.occlusionTexture = occlusionTexture,
		.emissiveFactor = material.emissiveFactor,
		.emissiveTexture = emissiveTexture};
}

struct GLB {
	struct Chunk {
		uint32 type;
		std::vector<uint8> data;
	};
	std::vector<Chunk> chunks;
	friend std::istream& operator>>(std::istream& is, GLB& glb) {
		glb.chunks.clear();

		uint32 magic, version, length;
		is.read(reinterpret_cast<char*>(&magic), 4);
		is.read(reinterpret_cast<char*>(&version), 4);
		is.read(reinterpret_cast<char*>(&length), 4);
		assert(magic == 0x46546C67);
		assert(version == 2);
		size_t pos = 12;

		while (pos < length) {
			Chunk chunk;
			uint32 chunkLength;
			is.read(reinterpret_cast<char*>(&chunkLength), 4);
			chunk.data.resize(chunkLength);
			is.read(reinterpret_cast<char*>(&chunk.type), 4);

			is.read(reinterpret_cast<char*>(chunk.data.data()), chunk.data.size());

			glb.chunks.push_back(chunk);

			pos += 8 + chunkLength;
		}

		return is;
	}
};

Model load_gltf(std::filesystem::path path, Render& render) {
	std::ifstream gltf_file(path, std::ifstream::binary);
	json j;
	std::vector<std::vector<uint8_t>> buffers_data;
	if (gltf_file.peek() == 'g') {
		GLB glb;
		gltf_file >> glb;
		j = json::parse(glb.chunks[0].data);
		if (glb.chunks.size() > 1)
			buffers_data.push_back(glb.chunks[1].data);
	} else {
		j = json::parse(gltf_file);
	}
	Gltf gltf = j.get<Gltf>();

	buffers_data.resize(gltf.buffers.size());
	for (size_t i = 0; i < buffers_data.size(); i++) {
		buffers_data[i].resize(gltf.buffers[i].byteLength);
		if (gltf.buffers[i].uri.has_value()) {
			load_uri(gltf.buffers[i].uri.value(), path, buffers_data[i]);
		}
	}

	std::vector<ImageData> images;
	images.resize(gltf.images.size());
	for (size_t i = 0; i < gltf.images.size(); i++) {
		const auto& image = gltf.images[i];
		std::vector<uint8> encoded_data;
		if (image.bufferView.has_value()) {
			const auto& bufferView = gltf.bufferViews[image.bufferView.value()];
			const auto& buffer = buffers_data[bufferView.buffer];

			const auto begin = buffer.begin() + bufferView.byteOffset;
			const auto end = begin + bufferView.byteLength;

			encoded_data = std::vector<uint8>(begin, end);
		}
		if (image.uri.has_value()) {
			load_uri(image.uri.value(), path, encoded_data);
		}

		auto& image_data = images[i];
		uint8* data = stbi_load_from_memory(
			encoded_data.data(), encoded_data.size(), &image_data.width, &image_data.height, &image_data.channels, 0);
		image_data.data = std::vector<uint8>(data, data + image_data.width * image_data.height * image_data.channels);
		stbi_image_free(data);
	}

	MaterialHandle default_material = render.create_pbr_material(convert_material(gltf, images, Gltf::Material{}));

	std::vector<MaterialHandle> materials;
	materials.reserve(gltf.materials.size());
	std::transform(
		gltf.materials.begin(), gltf.materials.end(), std::back_inserter(materials),
		[&render, &gltf, &images](const Gltf::Material& material) {
			return render.create_pbr_material(convert_material(gltf, images, material));
		});

	std::vector<std::vector<Model::Surface>> models;
	models.resize(gltf.meshes.size());
	for (size_t i = 0; i < models.size(); i++) {
		models[i].reserve(gltf.meshes[i].primitives.size());
		for (auto& prim : gltf.meshes[i].primitives) {
			if (!prim.attributes.position.has_value())
				continue;
			size_t count = gltf.accessors[prim.attributes.position.value()].count;
			bool has_position = prim.attributes.position.has_value();
			bool has_normal = prim.attributes.normal.has_value();
			bool has_tangent = prim.attributes.tangent.has_value();
			size_t num_uv = prim.attributes.texcoord.size();
			size_t num_colour = prim.attributes.color.size();
			StandardMesh mesh(
				count,
				{has_position, has_normal, has_tangent, has_tangent, num_uv > 0, num_uv > 1, num_uv > 2, num_uv > 3,
				 num_colour > 0, num_colour > 1});
			accessor_for_each(
				gltf, buffers_data, prim.attributes.position.value(), [&mesh](size_t vertex, const void* data) {
					mesh.position(vertex) = *reinterpret_cast<const glm::vec3*>(data);
				});

			if (prim.attributes.normal.has_value()) {
				accessor_for_each(
					gltf, buffers_data, prim.attributes.normal.value(), [&mesh](size_t vertex, const void* data) {
						mesh.normal(vertex) = *reinterpret_cast<const glm::vec3*>(data);
					});
			}

			if (prim.attributes.tangent.has_value()) {
				accessor_for_each(
					gltf, buffers_data, prim.attributes.tangent.value(), [&mesh](size_t vertex, const void* data) {
						const vec3& tangent = *reinterpret_cast<const glm::vec3*>(data);
						const float& sign =
							*reinterpret_cast<const float*>(reinterpret_cast<const glm::vec3*>(data) + 1);
						mesh.tangent(vertex) = tangent;
						mesh.bitangent(vertex) = (sign * cross(mesh.normal(vertex), tangent));
					});
			}

			for (size_t t = 0; t < prim.attributes.texcoord.size(); t++) {
				static const std::unordered_map<Gltf::ComponentType, std::function<vec2(const void*)>> convert_funcs = {
					{Gltf::ComponentType::FLOAT, [](const void* data) { return *reinterpret_cast<const vec2*>(data); }},
					{Gltf::ComponentType::UNSIGNED_BYTE,
					 [](const void* data) { return unpackUnorm2x8(*reinterpret_cast<const uint16*>(data)); }},
					{Gltf::ComponentType::UNSIGNED_SHORT,
					 [](const void* data) { return unpackUnorm2x16(*reinterpret_cast<const uint32*>(data)); }}};

				const Gltf::Accessor& accessor = gltf.accessors[prim.attributes.texcoord[t]];
				const std::function<vec2(const void*)> convert_func = convert_funcs.at(accessor.componentType);
				accessor_for_each(
					gltf, buffers_data, prim.attributes.texcoord[t],
					[&mesh, convert_func, t](size_t index, const void* data) {
						mesh.tex_coord(t, index) = convert_func(data);
					});
			}

			for (size_t c = 0; c < prim.attributes.color.size(); c++) {
				static const std::unordered_map<
					Gltf::Accessor::Type, std::unordered_map<Gltf::ComponentType, std::function<vec4(const void*)>>>
					convert_funcs = {
						{Gltf::Accessor::Type::VEC4,
						 {
							 {Gltf::ComponentType::FLOAT,
							  [](const void* data) { return *reinterpret_cast<const vec4*>(data); }},
							 {Gltf::ComponentType::UNSIGNED_BYTE,
							  [](const void* data) { return unpackUnorm4x8(*reinterpret_cast<const uint32*>(data)); }},
							 {Gltf::ComponentType::UNSIGNED_SHORT,
							  [](const void* data) { return unpackUnorm4x16(*reinterpret_cast<const uint64*>(data)); }},
						 }},
						{Gltf::Accessor::Type::VEC3,
						 {
							 {Gltf::ComponentType::FLOAT,
							  [](const void* data) { return vec4(*reinterpret_cast<const vec3*>(data), 1.0); }},
							 {Gltf::ComponentType::UNSIGNED_BYTE,
							  [](const void* data) {
								  return vec4((vec3(*reinterpret_cast<const u8vec3*>(data)) / 255.0f), 1.0f);
							  }},
							 {Gltf::ComponentType::UNSIGNED_SHORT,
							  [](const void* data) {
								  return vec4((vec3(*reinterpret_cast<const u16vec3*>(data)) / 65535.0f), 1.0f);
							  }},
						 }}};

				const Gltf::Accessor& accessor = gltf.accessors[prim.attributes.color[c]];
				const std::function<vec4(const void*)> convert_func =
					convert_funcs.at(accessor.type).at(accessor.componentType);
				accessor_for_each(
					gltf, buffers_data, prim.indices.value(), [&mesh, convert_func, c](size_t index, const void* data) {
						mesh.colour(c, index) = convert_func(data);
					});
			}

			if (prim.indices.has_value()) {
				static const std::unordered_map<Gltf::ComponentType, std::function<uint32(const void*)>> convert_funcs =
					{{Gltf::ComponentType::UNSIGNED_BYTE,
					  [](const void* data) { return *reinterpret_cast<const uint8*>(data); }},
					 {Gltf::ComponentType::UNSIGNED_SHORT,
					  [](const void* data) { return *reinterpret_cast<const uint16*>(data); }},
					 {Gltf::ComponentType::UNSIGNED_INT,
					  [](const void* data) { return *reinterpret_cast<const uint32*>(data); }}};

				const Gltf::Accessor& accessor = gltf.accessors[prim.indices.value()];
				mesh.indices.resize(accessor.count);
				const std::function<uint32_t(const void*)> convert_func = convert_funcs.at(accessor.componentType);
				accessor_for_each(
					gltf, buffers_data, prim.indices.value(), [&mesh, convert_func](size_t index, const void* data) {
						mesh.indices[index] = convert_func(data);
					});
			}

			MaterialHandle material = default_material;
			if (prim.material.has_value())
				material = materials.at(prim.material.value());

			models[i].push_back(Model::Surface{.mesh = render.standard_mesh_create(mesh), .material = material});
		}
	}

	Model model{.render = render, .surfaces = {}};

	if (gltf.scene.has_value())
		crawl_nodes(gltf, gltf.scenes[gltf.scene.value()].nodes, model.surfaces, models);

	return model;
}

} // namespace Render