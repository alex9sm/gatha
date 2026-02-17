#pragma once
#include "opengl.hpp"

namespace opengl {
	GLuint shader_create(const char* vert_path, const char* frag_path);
	void shader_destroy(GLuint program);
	bool shader_load(const char* folder);
	GLuint shader_get(const char* name);
	void shader_unload();

}