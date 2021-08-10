#include "core.hpp"

#include <algorithm>

#include "debug.hpp"
#include "gl.hpp"

namespace Render {
void Core::meshes_setup(size_t){};
void Core::meshes_cleanup(size_t handle) {
	auto& mesh = meshes_get(handle);
	glDeleteVertexArrays(1, &(mesh.vao));
	glDeleteBuffers(mesh.buffers.size(), mesh.buffers.data());
}

void Core::shaders_setup(size_t) {}
void Core::shaders_cleanup(size_t handle) {
	// assert(shaders_get(handle).materials.empty());
	glDeleteProgram(shaders_get(handle).shader);
}

void Core::materials_setup(size_t handle) {
	for (auto& shader_conf : materials_get(handle).shaders) {
		shaders_get(shader_conf.shader).materials.emplace(handle);
	}
}
void Core::materials_cleanup(size_t handle) {
	// assert(materials_get(handle).surfaces.empty());
	for (auto& shader_conf : materials_get(handle).shaders) {
		shaders_get(shader_conf.shader).materials.erase(handle);
	}
}

void Core::surfaces_setup(size_t handle) { materials_get(surfaces_get(handle).material).surfaces.emplace(handle); }
void Core::surfaces_cleanup(size_t handle) { materials_get(surfaces_get(handle).material).surfaces.erase(handle); }

Core::Core(void (*glGetProcAddr(const char*))()) {
	loadGL(glGetProcAddr);

	loadDebugger();

	glCreateBuffers(1, &cameraBuffer);
	glNamedBufferStorage(cameraBuffer, sizeof(Camera), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraBuffer);

	glCreateBuffers(1, &dirLightBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dirLightBuffer);

	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &dirLightShadow);
	glTextureStorage3D(dirLightShadow, 1, GL_DEPTH_COMPONENT32F, lightmapSize, lightmapSize, 8);
	glTextureParameteri(dirLightShadow, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTextureParameteri(dirLightShadow, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
}

uint Core::create_texture(int width, int height, int channels, TextureFlags flags, void* data) {
	bool srgb = flags & TextureFlags::SRGB;
	bool mipmaps = flags & TextureFlags::MIPMAPPED;
	GLuint texture;
	GLenum format, internalformat;
	switch (channels) {
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
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	auto levels = 1;
	if (mipmaps)
		levels = glm::max(glm::log2(min(width, height)), 1);
	glTextureStorage2D(texture, levels, internalformat, width, height);
	if (flags & TextureFlags::ANIOSTROPIC)
		glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, INFINITY);
	glTextureSubImage2D(texture, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
	if (flags & TextureFlags::CLAMPED) {
		glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	if (mipmaps)
		glGenerateTextureMipmap(texture);
	return texture;
}

void Core::renderScene(Shader::Type type, RenderOrder order) {
	if (order == RenderOrder::Shader) {
		for (auto& shader : shaders_dense) {
			if ((shader.type & type) == 0)
				continue;

			glUseProgram(shader.shader);
			for (auto mat : shader.materials) {
				auto& material = materials_get(mat);

				for (auto shaderConfig : material.shaders) {
					if (&shaders_get(shaderConfig.shader) != &shader)
						continue;
					glBindBufferBase(GL_UNIFORM_BUFFER, 1, shaderConfig.uniform);
					glBindTextures(3, shaderConfig.textures.size(), shaderConfig.textures.data());
					break;
				}

				for (auto& s : material.surfaces) {
					auto& surface = surfaces_get(s);
					glBindVertexArray(meshes_get(surface.mesh).vao);

					glUniformMatrix4fv(0, 1, false, value_ptr(surface.transform));

					glDrawElements(GL_TRIANGLES, meshes_get(surface.mesh).count, GL_UNSIGNED_INT, 0);
				}
			}
		}
	} else {
		for (auto& surface : surfaces_dense) {
			glBindVertexArray(meshes_get(surface.mesh).vao);
			for (auto& shader : materials_get(surface.material).shaders) {
				if ((shaders_get(shader.shader).type & type) == 0)
					continue;

				glUseProgram(shaders_get(shader.shader).shader);

				glBindBufferBase(GL_UNIFORM_BUFFER, 1, shader.uniform);
				glBindTextures(3, shader.textures.size(), shader.textures.data());

				glUniformMatrix4fv(0, 1, false, value_ptr(surface.transform));

				glDrawElements(GL_TRIANGLES, meshes_get(surface.mesh).count, GL_UNSIGNED_INT, 0);
			}
		}
	}
}

void Core::run() {
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	std::vector<DirLight> dirLights;
	this->dirLights.vector_push(dirLights);

	glNamedBufferData(dirLightBuffer, vector_size(dirLights), dirLights.data(), GL_DYNAMIC_DRAW);

	GLuint framebuffer;
	glCreateFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	for (size_t i = 0; i < dirLights.size(); i++) {
		glNamedFramebufferTextureLayer(framebuffer, GL_DEPTH_ATTACHMENT, dirLightShadow, 0, i);

		Camera cam = {.proj = mat4(1.0f), .view = dirLights[i].shadowMapTrans, .camPos = dirLights[i].dir};
		glNamedBufferSubData(cameraBuffer, 0, sizeof(Camera), &cam);

		glViewport(0, 0, lightmapSize, lightmapSize);
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		renderScene(Shader::Type::Shadow);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &framebuffer);

	Camera cam = {
		.proj = infinitePerspective(fov, static_cast<float>(width) / static_cast<float>(height), 0.1f),
		.view = cameraPos,
		.camPos = vec3(inverse(cameraPos) * vec4{0, 0, 0, 1})};
	glNamedBufferSubData(cameraBuffer, 0, sizeof(Camera), &cam);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindTextures(0, 1, &dirLightShadow);
	glCullFace(GL_BACK);
	renderScene(Shader::Type::Opaque);
}

} // namespace Render