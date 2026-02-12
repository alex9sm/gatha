#pragma once

#include "../core/types.hpp"

namespace renderer {

	bool init(void* native_window_handle, u32 window_width, u32 window_height);
	void shutdown();
	bool begin_frame();
	void end_frame();
	void on_resize(u32 width, u32 height);

}
