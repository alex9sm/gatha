#include "gatha.hpp"
#include "../core/types.hpp"
#include "../core/math.hpp"
#include "../core/memory.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/opengl/shader.hpp"
#include "../renderer/opengl/mesh.hpp"
#include "../platform/platform.hpp"

namespace {
    opengl::Mesh triangle;
    opengl::GLuint triangle_program;
}

bool init() {
	u32 w, h;
	platform::get_paint_field_size(&w, &h);
	renderer::init(platform::get_native_window_handle(), w, h);
	opengl::shader_load("shaders");
	triangle_program = opengl::shader_get("shader");

	opengl::Vertex verts[] = {
		 { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
		 { { 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
		 { { 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
	};
	u32 indices[] = { 0, 1, 2 };
	triangle = opengl::mesh_create(verts, 3, indices, 3);

	return true;
}

void update() {
	//TODO: game logic and input
}

void render() {
	renderer::begin_frame();
	mat4 mvp = mat4_identity();

	opengl::glUseProgram(triangle_program);
	opengl::GLint mvp_loc = opengl::glGetUniformLocation(triangle_program, "u_mvp");
	opengl::glUniformMatrix4fv(mvp_loc, 1, opengl::GL_FALSE, &mvp.col[0][0]);

	opengl::mesh_draw(triangle);
	renderer::end_frame();
}

void shutdown() {
	opengl::mesh_destroy(&triangle);
	opengl::shader_unload();
}