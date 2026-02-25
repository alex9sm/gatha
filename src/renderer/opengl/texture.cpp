#define STB_IMAGE_IMPLEMENTATION
#include "../../../dependencies/stb_image.h"

#include "texture.hpp"
#include "../../core/log.hpp"

namespace opengl {

	static GLsizei calc_mip_levels(GLsizei w, GLsizei h) {
		GLsizei levels = 1;
		while (w > 1 || h > 1) { w >>= 1; h >>= 1; levels++; }
		return levels;
	}

	GLuint texture_load(const char* filepath) {
		int w, h, channels;
		stbi_set_flip_vertically_on_load(1);
		u8* pixels = stbi_load(filepath, &w, &h, &channels, 0);
		if (!pixels) {
			logger::error("texture: failed to load '%s'", filepath);
			return 0;
		}

		GLenum internal_fmt = (channels == 4) ? GL_SRGB8_ALPHA8 : GL_SRGB8;
		GLenum upload_fmt = (channels == 4) ? GL_RGBA : GL_RGB;
		GLsizei mip_levels = calc_mip_levels((GLsizei)w, (GLsizei)h);

		GLuint tex;
		glCreateTextures(GL_TEXTURE_2D, 1, &tex);
		glTextureStorage2D(tex, mip_levels, internal_fmt, (GLsizei)w, (GLsizei)h);
		glTextureSubImage2D(tex, 0, 0, 0, (GLsizei)w, (GLsizei)h, upload_fmt, GL_UNSIGNED_BYTE, pixels);
		glGenerateTextureMipmap(tex);

		glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);

		stbi_image_free(pixels);
		logger::info("texture: loaded '%s' (%dx%d, %dch)", filepath, w, h, channels);
		return tex;
	}

	GLuint texture_create_solid(u8 r, u8 g, u8 b, u8 a) {
		u8 pixel[4] = { r, g, b, a };
		GLuint tex;
		glCreateTextures(GL_TEXTURE_2D, 1, &tex);
		glTextureStorage2D(tex, 1, GL_RGBA8, 1, 1);
		glTextureSubImage2D(tex, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
		glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		return tex;
	}

	void texture_destroy(GLuint tex) {
		if (tex) glDeleteTextures(1, &tex);
	}

}
