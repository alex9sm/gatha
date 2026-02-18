#pragma once
#include "../core/types.hpp"

namespace platform {

	int run();
	bool is_running();
	void* get_native_window_handle();
	void get_paint_field_size(u32* width, u32* height);
	f32 get_delta_time();

	enum Key : u32 {
        KEY_W,
        KEY_A,
        KEY_S,
        KEY_D,
        KEY_CTRL,
        KEY_SPACE,
        KEY_SHIFT,
        KEY_ESCAPE,
        KEY_COUNT
	};

    bool is_key_down(Key key);
    void get_mouse_delta(f32* dx, f32* dy);
    void set_mouse_captured(bool captured);
}