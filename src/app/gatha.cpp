#include "gatha.hpp"
#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "../renderer/renderer.hpp"
#include "../platform/platform.hpp"


bool init() {
	u32 w, h;
	platform::get_paint_field_size(&w, &h);
	renderer::init(platform::get_native_window_handle(), w, h);

	return true;
}

void update() {
	//TODO: game logic and input
}

void render() {
	renderer::begin_frame();
	// TODO: draw scene
	renderer::end_frame();
}

void shutdown() {
	//TODO: free everything
		
}