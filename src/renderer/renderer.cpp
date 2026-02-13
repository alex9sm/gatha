#include "renderer.hpp"
#include "opengl/opengl.hpp"

namespace renderer {

	bool init(void* native_window_handle, u32 window_width, u32 window_height) {
		return opengl::init(native_window_handle, window_width, window_height);
	}

	void shutdown() {
		opengl::shutdown();
	}

	bool begin_frame() {
		// TODO:
		opengl::glClear(opengl::GL_COLOR_BUFFER_BIT | opengl::GL_DEPTH_BUFFER_BIT);
		return true;
	}

	void end_frame() {
		// TODO: implement when presentation is ready
		opengl::swap_buffers();
	}

	void on_resize(u32 width, u32 height) {
		opengl::set_viewport(width, height);
	}

}
