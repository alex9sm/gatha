#include "renderer.hpp"
#include "vulkan/vk_core.hpp"

namespace renderer {

	bool init(void* native_window_handle, u32 window_width, u32 window_height) {
		return vulkan::init(native_window_handle, window_width, window_height);
	}

	void shutdown() {
		vulkan::shutdown();
	}

	bool begin_frame() {
		// TODO:
		return true;
	}

	void end_frame() {
		// TODO: implement when presentation is ready
	}

	void on_resize(u32 width, u32 height) {
		vulkan::recreate_swapchain(width, height);
	}

}
