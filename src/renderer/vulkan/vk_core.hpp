/*

#pragma once

#include "../../core/types.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#include <vulkan/vulkan.h>
#include <windows.h>

namespace renderer::vulkan {

	struct VulkanContext {

		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkSurfaceKHR surface;
		VkDevice device;
		VkPhysicalDeviceProperties device_properties;
		VkPhysicalDeviceFeatures device_features;
		VkPhysicalDeviceMemoryProperties device_memory;

		u32 graphics_queue_index;
		u32 present_queue_index;
		VkQueue graphics_queue;
		VkQueue present_queue;

		VkSwapchainKHR swapchain;
		VkFormat swapchain_format;
		VkExtent2D swapchain_extent;

	};

	VulkanContext* get_context();

	bool create_swapchain(u32 width, u32 height);
	void destroy_swapchain();
	bool recreate_swapchain(u32 width, u32 height);

	bool init(void* native_window_handle, u32 width, u32 height);
	void shutdown();

} */