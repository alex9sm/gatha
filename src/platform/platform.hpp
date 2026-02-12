#pragma once
#include "../core/types.hpp"

namespace platform {

	enum class WindowMode {Editor,Game};
	void set_window_mode(WindowMode mode);
	WindowMode get_window_mode();

	int run();
	bool is_running();
	void* get_native_window_handle();
	void get_paint_field_size(u32* width, u32* height);

}