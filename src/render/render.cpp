#include "render.hpp"

#include <string>

#include "gl.hpp"
#include "shaders.hpp"

namespace Render {

struct SprivStage {
	const std::vector<uint32_t>& data;
	GLenum shaderType;
	std::string entryPoint = "main";
};

GLuint load_spirv_program(const std::vector<SprivStage> stages) {
	GLuint program = glCreateProgram();

	for (auto& stage : stages) {
		GLuint shader = glCreateShader(stage.shaderType);
		glShaderBinary(
			1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, stage.data.data(), stage.data.size() * sizeof(uint32_t));
		glSpecializeShader(shader, stage.entryPoint.c_str(), 0, nullptr, nullptr);
		glAttachShader(program, shader);
	}

	glLinkProgram(program);

	return program;
}

struct PBR {
	vec4 albedoFactor;
	vec3 emissiveFactor;
	float metalFactor;
	float roughFactor;
	float reflectionLevels;
	uint has_albedo_texture;
	uint has_metal_rough_texture;
	uint has_normal_texture;
	uint has_occlusion_texture;
	uint has_emissive_texture;
	float alpha_depth_cutoff;
};
MaterialHandle Render::create_pbr_material(MaterialPBR pbr) {
	static ShaderHandle shader = shaders_insert(Shader{
		load_spirv_program({{Shaders::default_vert, GL_VERTEX_SHADER}, {Shaders::pbr_frag, GL_FRAGMENT_SHADER}}),
		Shader::Type::Opaque});
	static ShaderHandle trans_shader = shaders_insert(Shader{
		load_spirv_program({{Shaders::default_vert, GL_VERTEX_SHADER}, {Shaders::pbr_frag, GL_FRAGMENT_SHADER}}),
		Shader::Type::Transparent});
	static ShaderHandle depthShader = shaders_insert(Shader{
		load_spirv_program({{Shaders::default_vert, GL_VERTEX_SHADER}, {Shaders::depth_frag, GL_FRAGMENT_SHADER}}),
		Shader::Type::Depth | Shader::Type::Shadow});
	static ShaderHandle cutoffDepthShader = shaders_insert(Shader{
		load_spirv_program(
			{{Shaders::default_vert, GL_VERTEX_SHADER}, {Shaders::cutoff_depth_frag, GL_FRAGMENT_SHADER}}),
		Shader::Type::Depth | Shader::Type::Shadow});

	GLuint uniform;
	glCreateBuffers(1, &uniform);
	PBR _pbr = {
		.albedoFactor = pbr.albedoFactor,
		.emissiveFactor = pbr.emissiveFactor,
		.metalFactor = pbr.metalFactor,
		.roughFactor = pbr.roughFactor,
		.reflectionLevels = static_cast<float>(reflectionLevels),
		.has_albedo_texture = pbr.albedoTexture.has_value(),
		.has_metal_rough_texture = pbr.metalRoughTexture.has_value(),
		.has_normal_texture = pbr.normalTexture.has_value(),
		.has_occlusion_texture = pbr.occlusionTexture.has_value(),
		.has_emissive_texture = pbr.emissiveTexture.has_value(),
		.alpha_depth_cutoff = pbr.alphaCutoff,
	};
	glNamedBufferStorage(uniform, sizeof(PBR), &_pbr, 0);

	std::vector<TextureHandle> textures = {
		irradiance,
		reflection,
		reflectionBRDF,
		pbr.albedoTexture.value_or(0),
		pbr.metalRoughTexture.value_or(0),
		pbr.normalTexture.value_or(0),
		pbr.occlusionTexture.value_or(0),
		pbr.emissiveTexture.value_or(0)};

	switch (pbr.alphaMode) {
	case MaterialPBR::AlphaMode::Opaque:
		return materials_insert(Material{{
			{.shader = shader, .uniform = uniform, .textures = textures},
			{.shader = depthShader, .uniform = 0, .textures = {}},
		}});
	case MaterialPBR::AlphaMode::Masked:
		return materials_insert(Material{{
			{.shader = shader, .uniform = uniform, .textures = textures},
			{.shader = cutoffDepthShader, .uniform = uniform, .textures = {pbr.albedoTexture.value_or(0)}},
		}});
	case MaterialPBR::AlphaMode::Blend:
		return materials_insert(Material{{
			{.shader = trans_shader, .uniform = uniform, .textures = textures},
		}});
	}
}

Render::Render(void (*glGetProcAddr(const char*))()) : Core(glGetProcAddr) {
	{
		ShaderHandle skyboxShader = shaders_insert(Shader{
			load_spirv_program(
				{{Shaders::skybox_vert, GL_VERTEX_SHADER}, {Shaders::test_skybox_frag, GL_FRAGMENT_SHADER}}),
			Shader::Type::Opaque | Shader::Type::Skybox});
		MaterialHandle skyboxMaterial =
			materials_insert(Material{{{.shader = skyboxShader, .uniform = 0, .textures = {}}}});

		std::vector<vec3> verticies = {
			{-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1, 1}, {1, -1, -1}, {1, -1, 1}, {1, 1, -1}, {1, 1, 1},
		};

		// clang-format off
	std::vector<uint32_t> indicies = {
		2, 0, 4, 4, 6, 2,
		1, 0, 2, 2, 3, 1,
		4, 5, 7, 7, 6, 4,
		1, 3, 7, 7, 5, 1,
		2, 6, 7, 7, 3, 2,
		0, 1, 4, 4, 1, 5,
	};
		// clang-format on

		GLuint vertex_buffer, index_buffer, vao;
		glCreateBuffers(1, &vertex_buffer);
		glCreateBuffers(1, &index_buffer);
		glCreateVertexArrays(1, &vao);

		glNamedBufferStorage(vertex_buffer, vector_size(verticies), verticies.data(), 0);
		glNamedBufferStorage(index_buffer, vector_size(indicies), indicies.data(), 0);

		glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, sizeof(vec3));
		glEnableVertexArrayAttrib(vao, 0);
		glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, false, 0);

		glVertexArrayElementBuffer(vao, index_buffer);

		MeshHandle skyboxMesh = meshes_insert(
			Mesh{.vao = vao, .count = static_cast<int32_t>(indicies.size()), .buffers = {vertex_buffer, index_buffer}});
		skybox = surface_create(skyboxMesh, skyboxMaterial);
	}

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &skyboxCubemap);
	glTextureStorage2D(skyboxCubemap, 1, GL_RGB16F, skyboxSize, skyboxSize);

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &irradiance);
	glTextureStorage2D(irradiance, 1, GL_RGB16F, irradianceSize, irradianceSize);

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &reflection);
	glTextureStorage2D(reflection, reflectionLevels, GL_RGB16F, reflectionSize, reflectionSize);
	glTextureParameteri(reflection, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glCreateTextures(GL_TEXTURE_2D, 1, &reflectionBRDF);
	glTextureStorage2D(reflectionBRDF, 1, GL_RG16F, reflectionBRDFSize, reflectionBRDFSize);
	glTextureParameteri(reflectionBRDF, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(reflectionBRDF, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	{
		GLuint framebuffer;
		glCreateFramebuffers(1, &framebuffer);
		glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, reflectionBRDF, 0);
		glViewport(0, 0, reflectionBRDFSize, reflectionBRDFSize);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		static GLuint reflectionBRDFShader = load_spirv_program(
			{{Shaders::quad_vert, GL_VERTEX_SHADER}, {Shaders::reflection_brdf_frag, GL_FRAGMENT_SHADER}});

		glUseProgram(reflectionBRDFShader);
		mat4 transform(1.0f);
		glUniformMatrix4fv(0, 1, false, value_ptr(transform));

		GLuint vertex_buffer, index_buffer, quadVertexArray;
		glCreateBuffers(1, &vertex_buffer);
		glCreateBuffers(1, &index_buffer);
		glCreateVertexArrays(1, &quadVertexArray);

		vec2 quadverts[] = {
			{-1, -1}, {0, 0}, {-1, 1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}, {1, 1},
		};
		int quadindicies[] = {0, 1, 2, 2, 1, 3};

		glNamedBufferStorage(vertex_buffer, sizeof(quadverts), &quadverts, 0);
		glNamedBufferStorage(index_buffer, sizeof(quadindicies), &quadindicies, 0);

		glVertexArrayVertexBuffer(quadVertexArray, 0, vertex_buffer, 0, sizeof(vec2) * 2);
		glEnableVertexArrayAttrib(quadVertexArray, 0);
		glVertexArrayAttribFormat(quadVertexArray, 0, 2, GL_FLOAT, false, 0);
		glVertexArrayVertexBuffer(quadVertexArray, 1, vertex_buffer, 0, sizeof(vec2) * 2);
		glEnableVertexArrayAttrib(quadVertexArray, 1);
		glVertexArrayAttribFormat(quadVertexArray, 1, 2, GL_FLOAT, false, sizeof(vec2));

		glVertexArrayElementBuffer(quadVertexArray, index_buffer);

		glBindVertexArray(quadVertexArray);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDeleteProgram(reflectionBRDFShader);
		glDeleteBuffers(1, &vertex_buffer);
		glDeleteBuffers(1, &index_buffer);
		glDeleteVertexArrays(1, &quadVertexArray);
	}

	{
		const float zero_block[4] = {0, 0, 0, 0};
		glCreateBuffers(1, &zero_buffer);
		glNamedBufferStorage(zero_buffer, sizeof(zero_block), zero_block, 0);
	}
}

const glm::mat4 captureViews[] = {
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

void Render::render_cubemap(Shader::Type type, RenderOrder order, GLuint cubemap, GLsizei size) {
	GLuint framebuffer, renderbuffer;
	glCreateFramebuffers(1, &framebuffer);
	glCreateRenderbuffers(1, &renderbuffer);
	glNamedRenderbufferStorage(renderbuffer, GL_DEPTH_COMPONENT24, size, size);
	glNamedFramebufferRenderbuffer(framebuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	for (int i = 0; i < 6; i++) {
		glNamedFramebufferTextureLayer(framebuffer, GL_COLOR_ATTACHMENT0, cubemap, 0, i);

		Camera cam = {
			.proj = infinitePerspective(glm::radians(90.0f), 1.0f, 0.1f),
			.view = captureViews[i],
			.camPos = vec3(0.0f)};
		glNamedBufferSubData(cameraBuffer, 0, sizeof(Camera), &cam);

		glViewport(0, 0, size, size);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderScene(type, order);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteRenderbuffers(1, &renderbuffer);
}

void Render::update_skybox() {
	render_cubemap(Shader::Type::Skybox, RenderOrder::Shader, skyboxCubemap, skyboxSize);

	GLuint framebuffer;
	glCreateFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glBindVertexArray(meshes_get(surfaces_get(skybox).mesh).vao);

	static GLuint irradianceShader =
		load_spirv_program({{Shaders::skybox_vert, GL_VERTEX_SHADER}, {Shaders::irradiance_frag, GL_FRAGMENT_SHADER}});
	glUseProgram(irradianceShader);
	glBindTextures(1, 1, &skyboxCubemap);

	for (int i = 0; i < 6; i++) {
		glNamedFramebufferTextureLayer(framebuffer, GL_COLOR_ATTACHMENT0, irradiance, 0, i);

		Camera cam = {
			.proj = infinitePerspective(glm::radians(90.0f), 1.0f, 0.1f),
			.view = captureViews[i],
			.camPos = vec3(0.0f)};
		glNamedBufferSubData(cameraBuffer, 0, sizeof(Camera), &cam);

		glViewport(0, 0, irradianceSize, irradianceSize);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, meshes_get(surfaces_get(skybox).mesh).count, GL_UNSIGNED_INT, 0);
	}

	static GLuint reflectionShader =
		load_spirv_program({{Shaders::skybox_vert, GL_VERTEX_SHADER}, {Shaders::reflection_frag, GL_FRAGMENT_SHADER}});
	glUseProgram(reflectionShader);
	glBindTextures(1, 1, &skyboxCubemap);

	for (int level = 0; level < reflectionLevels; level++) {
		glUniform1f(1, static_cast<float>(level) / static_cast<float>(reflectionLevels - 1));

		int levelSize = reflectionSize * pow(0.5, level);
		glViewport(0, 0, levelSize, levelSize);
		for (int i = 0; i < 6; i++) {
			glNamedFramebufferTextureLayer(framebuffer, GL_COLOR_ATTACHMENT0, reflection, level, i);

			Camera cam = {
				.proj = infinitePerspective(glm::radians(90.0f), 1.0f, 0.1f),
				.view = captureViews[i],
				.camPos = vec3(0.0f)};
			glNamedBufferSubData(cameraBuffer, 0, sizeof(Camera), &cam);

			glClear(GL_COLOR_BUFFER_BIT);
			glDrawElements(GL_TRIANGLES, meshes_get(surfaces_get(skybox).mesh).count, GL_UNSIGNED_INT, 0);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &framebuffer);

	// set_skybox_rect_texture(reflectionBRDF, false);
}

void Render::set_skybox_rect_texture(TextureHandle texture, bool update) {
	static ShaderHandle skyboxShader = shaders_insert(Shader{
		load_spirv_program({{Shaders::skybox_vert, GL_VERTEX_SHADER}, {Shaders::rect_skybox_frag, GL_FRAGMENT_SHADER}}),
		Shader::Type::Opaque | Shader::Type::Skybox});
	static ShaderHandle skyboxDepthShader = shaders_insert(Shader{
		load_spirv_program({{Shaders::skybox_vert, GL_VERTEX_SHADER}, {Shaders::depth_frag, GL_FRAGMENT_SHADER}}),
		Shader::Type::Depth});
	static MaterialHandle skyboxMaterial = materials_insert(Material{
		{{.shader = skyboxShader, .uniform = 0, .textures = {texture}},
		 {.shader = skyboxDepthShader, .uniform = 0, .textures = {}}}});
	materials_get(skyboxMaterial).shader_passes[0].textures[0] = texture;
	set_skybox_material(skyboxMaterial, update);
}

void Render::set_skybox_cube_texture(TextureHandle texture, bool update) {
	static ShaderHandle skyboxShader = shaders_insert(Shader{
		load_spirv_program({{Shaders::skybox_vert, GL_VERTEX_SHADER}, {Shaders::cube_skybox_frag, GL_FRAGMENT_SHADER}}),
		Shader::Type::Opaque | Shader::Type::Skybox});
	static ShaderHandle skyboxDepthShader = shaders_insert(Shader{
		load_spirv_program({{Shaders::skybox_vert, GL_VERTEX_SHADER}, {Shaders::depth_frag, GL_FRAGMENT_SHADER}}),
		Shader::Type::Depth});
	static MaterialHandle skyboxMaterial = materials_insert(Material{
		{{.shader = skyboxShader, .uniform = 0, .textures = {texture}},
		 {.shader = skyboxDepthShader, .uniform = 0, .textures = {}}}});
	materials_get(skyboxMaterial).shader_passes[0].textures[0] = texture;
	set_skybox_material(skyboxMaterial, update);
}

MeshHandle Render::standard_mesh_create(StandardMesh mesh) {
	if (!mesh.format.has_normal)
		mesh.gen_normals();

	if (!mesh.format.has_tangent && mesh.format.has_tex_coord_0)
		mesh.gen_tangents();

	mesh.reindex();

	GLuint vertex_buffer, index_buffer, vao;
	glCreateBuffers(1, &vertex_buffer);
	glCreateBuffers(1, &index_buffer);
	glCreateVertexArrays(1, &vao);

	glNamedBufferStorage(vertex_buffer, mesh.vertex_data.size() * sizeof(float), mesh.vertex_data.data(), 0);
	glNamedBufferStorage(index_buffer, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), 0);

	glVertexArrayElementBuffer(vao, index_buffer);
	glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, mesh.stride * sizeof(float));
	glVertexArrayVertexBuffer(vao, 15, zero_buffer, 0, 0);

	uint attrib_index = 0;

#define STANDARD_MESH_VERTEX_FEILD(name, size)                                                                         \
	if (mesh.format.has_##name) {                                                                                      \
		glEnableVertexArrayAttrib(vao, attrib_index);                                                                  \
		glVertexArrayAttribFormat(vao, attrib_index, size, GL_FLOAT, false, mesh.offset_##name * sizeof(float));       \
		glVertexArrayAttribBinding(vao, attrib_index, 0);                                                              \
	} else {                                                                                                           \
		glEnableVertexArrayAttrib(vao, attrib_index);                                                                  \
		glVertexArrayAttribFormat(vao, attrib_index, size, GL_FLOAT, false, 0);                                        \
		glVertexArrayAttribBinding(vao, attrib_index, 15);                                                             \
	}                                                                                                                  \
	attrib_index++;
	STANDARD_MESH_VERTEX_FORMAT
#undef STANDARD_MESH_VERTEX_FEILD

	return meshes_insert(
		Mesh{.vao = vao, .count = static_cast<int>(mesh.indices.size()), .buffers = {vertex_buffer, index_buffer}});
}

} // namespace Render