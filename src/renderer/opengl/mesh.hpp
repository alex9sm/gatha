#pragma once

#include "opengl.hpp"
#include "vertex.hpp"

namespace opengl {

	struct Mesh {
		GLuint vao;
		GLuint vbo;
		GLuint ibo;
		u32 index_count;
	};

	Mesh mesh_create(const Vertex* vertices, u32 vertex_count, const u32* indices, u32 index_count);
	void mesh_destroy(Mesh* mesh);
	void mesh_draw(const Mesh& mesh);
}