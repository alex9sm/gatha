#include "gatha.hpp"
#include "camera.hpp"
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
    opengl::GLint mvp_loc;
	Camera cam;
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

	camera_init(&cam, { 0.0f, 0.0f, 3.0f }, 5.0f, 0.002f);
	platform::set_mouse_captured(true);
	mvp_loc = opengl::glGetUniformLocation(triangle_program, "u_mvp");

	return true;
}

void update() {
	f32 dt = platform::get_delta_time();
	static f32 esc_cooldown = 0.0f;
	esc_cooldown -= dt;
	if (platform::is_key_down(platform::KEY_ESCAPE) && esc_cooldown <= 0.0f) {
		static bool captured = true;
		captured = !captured;
		platform::set_mouse_captured(captured);
		esc_cooldown = 0.3f;
	}

	camera_update(&cam, dt);
}

void render() {
	renderer::begin_frame();
	u32 w, h;
	platform::get_paint_field_size(&w, &h);
	f32 aspect = (h > 0) ? (f32)w / (f32)h : 1.0f;

	mat4 model = mat4_identity();
	mat4 view = camera_get_view(&cam);
	mat4 proj = mat4_perspective(to_radians(60.0f), aspect, 0.1f, 1000.0f);

	mat4 mvp = proj * view * model;

	opengl::glUseProgram(triangle_program);
	opengl::glUniformMatrix4fv(mvp_loc, 1, opengl::GL_FALSE, &mvp.col[0][0]);

	opengl::mesh_draw(triangle);
	renderer::end_frame();
}

void shutdown() {
	platform::set_mouse_captured(false);
	opengl::mesh_destroy(&triangle);
	opengl::shader_unload();
}