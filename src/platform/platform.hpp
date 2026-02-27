#pragma once
#include "../core/types.hpp"
#include "../core/file.hpp"

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
        KEY_F5,
        KEY_COUNT
	};

    bool is_key_down(Key key);
    void get_mouse_delta(f32* dx, f32* dy);
    void set_mouse_captured(bool captured);
    bool is_mouse_captured();

    void editor_init();
    void editor_toggle();
    bool is_editor_mode();
    void editor_set_fps(f32 fps, f32 frame_time_ms);
    void editor_set_menu_callback(void (*callback)(int));
    bool editor_open_file_dialog(char* out_path, u32 max_len);
    bool editor_save_file_dialog(char* out_path, u32 max_len);

    void editor_set_asset_entries(const arr::Array<file::FileEntry>* entries);

    constexpr int MENU_FILE_SAVE    = 40001;
    constexpr int MENU_FILE_SAVE_AS = 40002;
    constexpr int MENU_FILE_LOAD    = 40003;
}