#pragma once
#include "../core/types.hpp"

namespace platform {

	int run();
	bool is_running();
	void* get_native_window_handle();
	void get_paint_field_size(u32* width, u32* height);

}