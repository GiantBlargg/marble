#include "import.hpp"

#include "engine.hpp"
#include <gl.hpp>
#include <stb_image.h>

namespace Render {

TextureHandle importSkybox(std::string filename) {
	int width, height;
	float* data = stbi_loadf(filename.c_str(), &width, &height, nullptr, 3);

	GLuint texture;

	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureStorage2D(texture, 1, GL_RGB16F, width, height);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, data);

	stbi_image_free(data);

	return texture;
}

} // namespace Render