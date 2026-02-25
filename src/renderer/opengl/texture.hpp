#pragma once

#include "opengl.hpp"

namespace opengl {

	GLuint texture_load(const char* filepath);
	GLuint texture_create_solid(u8 r, u8 g, u8 b, u8 a);
	void   texture_destroy(GLuint tex);

}
