/*
#include "vk_core.hpp"
#include "../../core/log.hpp"
#include "../../core/array.hpp"

namespace renderer::vulkan {

	namespace {
		VulkanContext context = {};
		bool initialized = false;
	}

	VulkanContext* get_context() {
		return &context;
	}

	static bool check_success(VkResult result, const char* operation) {
		if (result != VK_SUCCESS) {
			logger::error("Vulkan: %s failed with error code %d", operation, result);
			return false;
		}
		return true;
	}

	static bool create_instance() {
		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Gatha";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "Gatha Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_4;

		const char* extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
		};

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		create_info.enabledExtensionCount = 2;
		create_info.ppEnabledExtensionNames = extensions;

		VkResult result = vkCreateInstance(&create_info, nullptr, &context.instance);
		return check_success(result, "vkCreateInstance");
	}

	static bool create_surface(void* window_handle) {
		HWND hwnd = static_cast<HWND>(window_handle);
		HINSTANCE hInstance = GetModuleHandle(nullptr);
		VkWin32SurfaceCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		create_info.hwnd = hwnd;
		create_info.hinstance = hInstance;

		VkResult result = vkCreateWin32SurfaceKHR(context.instance, &create_info, nullptr, &context.surface);
		return check_success(result, "vkCreateWin32SurfaceKH");
	}

	static bool select_device() {
		u32 device_count = 0;
		vkEnumeratePhysicalDevices(context.instance, &device_count, nullptr);

		if (device_count == 0) {
			logger::error("No vulkan compatible GPUs found.");
			return false;
		}

		arr::Array<VkPhysicalDevice> devices = arr::array_create<VkPhysicalDevice>();
		arr::array_resize(&devices, device_count);
		vkEnumeratePhysicalDevices(context.instance, &device_count, devices.data);

		for (u32 i = 0; i < device_count; i++) {
			VkPhysicalDevice device = devices.data[i];
			u32 queue_family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
			arr::Array<VkQueueFamilyProperties> queue_families = arr::array_create<VkQueueFamilyProperties>();
			arr::array_resize(&queue_families, queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data);
			u32 graphics_family = UINT32_MAX;
			u32 present_family = UINT32_MAX;

			for (u32 j = 0; j < queue_family_count; j++) {
				if (queue_families.data[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphics_family = j;
				}
				VkBool32 present_support = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, j, context.surface, &present_support);
				if (present_support) {
					present_family = j;
				}
				if (graphics_family != UINT32_MAX && present_family != UINT32_MAX) {
					break;
				}
			}

			arr::array_destroy(&queue_families);

			if (graphics_family != UINT32_MAX && present_family != UINT32_MAX) {
				context.physicalDevice = device;
				context.graphics_queue_index = graphics_family;
				context.present_queue_index = present_family;
				vkGetPhysicalDeviceProperties(device, &context.device_properties);
				vkGetPhysicalDeviceFeatures(device, &context.device_features);
				vkGetPhysicalDeviceMemoryProperties(device, &context.device_memory);
				logger::info("Vulkan selected GPU: %s", context.device_properties.deviceName);
				arr::array_destroy(&devices);
				return true;
			}
		}

		arr::array_destroy(&devices);
		logger::error("No vulkan compatible GPU found.");
		return false;
	}

	static bool create_logical_device() {
		float priority = 1.0f;

		arr::Array<VkDeviceQueueCreateInfo> queue_create_infos = arr::array_create<VkDeviceQueueCreateInfo>();
		VkDeviceQueueCreateInfo graphics_queue_info = {};
		graphics_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphics_queue_info.queueFamilyIndex = context.graphics_queue_index;
		graphics_queue_info.queueCount = 1;
		graphics_queue_info.pQueuePriorities = &priority;
		arr::array_push(&queue_create_infos, graphics_queue_info);

		if (context.present_queue_index != context.graphics_queue_index) {
			VkDeviceQueueCreateInfo present_queue_info = {};
			present_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			present_queue_info.queueFamilyIndex = context.present_queue_index;
			present_queue_info.queueCount = 1;
			present_queue_info.pQueuePriorities = &priority;
			arr::array_push(&queue_create_infos, present_queue_info);
		}

		const char* extensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VkDeviceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.queueCreateInfoCount = static_cast<u32>(queue_create_infos.count);
		create_info.pQueueCreateInfos = queue_create_infos.data;
		create_info.enabledExtensionCount = 1;
		create_info.ppEnabledExtensionNames = extensions;

		VkResult result = vkCreateDevice(context.physicalDevice, &create_info, nullptr, &context.device);
		arr::array_destroy(&queue_create_infos);

		if (!check_success(result, "vkCreateDevice")) {
			return false;
		}
		vkGetDeviceQueue(context.device, context.graphics_queue_index, 0, &context.graphics_queue);
		vkGetDeviceQueue(context.device, context.present_queue_index, 0, &context.present_queue);

		return true;
	}

	bool create_swapchain(u32 width, u32 height) {
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &capabilities);
		u32 format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &format_count, nullptr);
		arr::Array<VkSurfaceFormatKHR> formats = arr::array_create<VkSurfaceFormatKHR>();
		arr::array_resize(&formats, format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &format_count, formats.data);
		u32 present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &present_mode_count, nullptr);
		arr::Array<VkPresentModeKHR> present_modes = arr::array_create<VkPresentModeKHR>();
		arr::array_resize(&present_modes, present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &present_mode_count, present_modes.data);
		VkSurfaceFormatKHR chosen_format = formats.data[0];

		for (u32 i = 0; i < format_count; i++) {
			if (formats.data[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
				formats.data[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				chosen_format = formats.data[i];
				break;
			}
		}

		VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
		VkExtent2D chosen_extent;
		if (capabilities.currentExtent.width != UINT32_MAX) {
			chosen_extent = capabilities.currentExtent;
		}
		else {
			chosen_extent.width = width;
			chosen_extent.height = height;
			if (chosen_extent.width < capabilities.minImageExtent.width)
				chosen_extent.width = capabilities.minImageExtent.width;
			if (chosen_extent.width > capabilities.maxImageExtent.width)
				chosen_extent.width = capabilities.maxImageExtent.width;
			if (chosen_extent.height < capabilities.minImageExtent.height)
				chosen_extent.height = capabilities.minImageExtent.height;
			if (chosen_extent.height > capabilities.maxImageExtent.height)
				chosen_extent.height = capabilities.maxImageExtent.height;
		}

		u32 image_count = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
			image_count = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = context.surface;
		create_info.minImageCount = image_count;
		create_info.imageFormat = chosen_format.format;
		create_info.imageColorSpace = chosen_format.colorSpace;
		create_info.imageExtent = chosen_extent;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		u32 queue_family_indices[] = { context.graphics_queue_index, context.present_queue_index };
		if (context.graphics_queue_index != context.present_queue_index) {
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else {
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		create_info.preTransform = capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = chosen_present_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = VK_NULL_HANDLE;
		VkResult result = vkCreateSwapchainKHR(context.device, &create_info, nullptr, &context.swapchain);
		arr::array_destroy(&formats);
		arr::array_destroy(&present_modes);

		if (!check_success(result, "vkCreateSwapchainKHR")) {
			return false;
		}
		context.swapchain_format = chosen_format.format;
		context.swapchain_extent = chosen_extent;

		return true;
	}

	void destroy_swapchain() {
		if (context.swapchain) {
			vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
			context.swapchain = VK_NULL_HANDLE;
		}
	}

	bool recreate_swapchain(u32 width, u32 height) {
		vkDeviceWaitIdle(context.device);
		destroy_swapchain();
		return create_swapchain(width, height);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	bool init(void* native_window_handle, u32 width, u32 height) {
		logger::info("initializing vulkan");

		if (!create_instance()) return false;
		if (!create_surface(native_window_handle)) return false;
		if (!select_device()) return false;
		if (!create_logical_device()) return false;
		if (!create_swapchain(width, height)) return false;
		initialized = true;
		logger::info("vulkan initialized");
		return true;
	}

	void shutdown() {
		if (!initialized) return;
		logger::info("vulkan shutting down");

		if (context.device) {
			vkDeviceWaitIdle(context.device);
		}
		destroy_swapchain();
		if (context.device) {
			vkDestroyDevice(context.device, nullptr);
			context.device = VK_NULL_HANDLE;
		}
		if (context.surface) {
			vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
			context.surface = VK_NULL_HANDLE;
		}
		if (context.instance) {
			vkDestroyInstance(context.instance, nullptr);
			context.instance = VK_NULL_HANDLE;
		}
		initialized = false;
		logger::info("vulkan has shutdown");
	}

} */