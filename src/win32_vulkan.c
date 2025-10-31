#include "win32_unity.h"

PFN_vkCreateInstance vkCreateInstance;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkGetDeviceQueue vkGetDeviceQueue;
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;

u32 u32_clamp(u32 min, u32 value, u32 max) {
	if (value > max) {
		return max;
	}

	if (value < min) {
		return min;
	}

	return value;
}

void win32_init_vulkan(HWND window, HINSTANCE instance, u32 window_width, u32 window_height) {
	// Load functions
	HMODULE vulkan_dll = LoadLibrary("vulkan-1");
	if (vulkan_dll == NULL) {
		printf("Failed to load vulkan dll\n");
		return;
	}

	vkCreateInstance = (PFN_vkCreateInstance)GetProcAddress(vulkan_dll, "vkCreateInstance");
	vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)GetProcAddress(vulkan_dll, "vkEnumerateInstanceLayerProperties");
	vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)GetProcAddress(vulkan_dll, "vkEnumeratePhysicalDevices");
	vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)GetProcAddress(vulkan_dll, "vkGetPhysicalDeviceProperties");
	vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)GetProcAddress(vulkan_dll, "vkGetPhysicalDeviceFeatures");
	vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)GetProcAddress(vulkan_dll, "vkGetPhysicalDeviceQueueFamilyProperties");
	vkCreateDevice = (PFN_vkCreateDevice)GetProcAddress(vulkan_dll, "vkCreateDevice");
	vkGetDeviceQueue = (PFN_vkGetDeviceQueue)GetProcAddress(vulkan_dll, "vkGetDeviceQueue");
	vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)GetProcAddress(vulkan_dll, "vkCreateWin32SurfaceKHR");
	vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)GetProcAddress(vulkan_dll, "vkGetPhysicalDeviceSurfaceSupportKHR");
	vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)GetProcAddress(vulkan_dll, "vkEnumerateDeviceExtensionProperties");
	vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)GetProcAddress(vulkan_dll, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)GetProcAddress(vulkan_dll, "vkGetPhysicalDeviceSurfacePresentModesKHR");
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)GetProcAddress(vulkan_dll, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)GetProcAddress(vulkan_dll, "vkCreateSwapchainKHR");

	// List extensions and layers
	const char* enabled_layers[] = {
		"VK_LAYER_KHRONOS_validation"
	};

	const char* instance_extensions[] = {
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME
	};

	const char* device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const u32 enabled_layers_count = sizeof(enabled_layers) / sizeof(char*);
	const u32 instance_extensions_count = sizeof(instance_extensions) / sizeof(char*);
	const u32 device_extension_count = sizeof(device_extensions) / sizeof(char*);

	// Create instance
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Pixel tracer",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &application_info,
		.enabledExtensionCount = instance_extensions_count,
		.ppEnabledExtensionNames = instance_extensions,
		.enabledLayerCount = enabled_layers_count,
		.ppEnabledLayerNames = enabled_layers
	};

	VkInstance vk_instance;
	if (vkCreateInstance(&create_info, NULL, &vk_instance) != VK_SUCCESS) {
		printf("Failed to create vulkan instance\n");
		return;
	}

	// Create surface
	VkWin32SurfaceCreateInfoKHR surface_create_info = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hwnd = window,
		.hinstance = instance
	};

	VkSurfaceKHR surface;
	if (vkCreateWin32SurfaceKHR(vk_instance, &surface_create_info, NULL, &surface) != VK_SUCCESS) {
		printf("Failed to create surface\n");
		return;
	}

	// Print available layers
	u32 layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);
	VkLayerProperties available_layers[layer_count];
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	printf("Available layers: %i\n", layer_count);
	for (u32 i = 0; i < layer_count; i++) {
		printf("%s\n", available_layers[i].layerName);
	}

	// Look for a suitable physical device
	u32 physical_device_count;
	vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, NULL);
	VkPhysicalDevice physical_devices[physical_device_count];
	vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices);

	VkPhysicalDevice physical_device;
	u32 graphics_family;
	u32 present_family;

	const u32 max_queue_family_count = 64;
	u32 queue_family_count;
	VkQueueFamilyProperties queue_families[max_queue_family_count];

	bool found_graphics_family = false;
	bool found_present_family = false;
	bool found_all_extensions = true;
	for (u32 i = 0; i < physical_device_count; i++) {
		// Check extensions
		const u32 max_available_extension_count = 512;
		u32 available_extension_count = max_available_extension_count;
		VkExtensionProperties available_extensions[max_available_extension_count];
		vkEnumerateDeviceExtensionProperties(physical_devices[i], NULL, &available_extension_count, available_extensions);
		assert(available_extension_count <= max_available_extension_count);

		FOR(required_extension_index, device_extension_count) {
			bool found_extension = false;

			FOR(available_extension_index, available_extension_count) {
				if (strcmp(available_extensions[available_extension_index].extensionName, device_extensions[required_extension_index]) == 0) {
					found_extension = true;
					break;
				}
			}

			if (!found_extension) {
				found_all_extensions = false;
				break;
			}
		}

		if (!found_all_extensions) {
			continue;
		}

		// Check queue families
		vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_families);
		assert(queue_family_count <= max_queue_family_count);
		for (u32 j = 0; j < queue_family_count; j++) {
			if (queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
				queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
				found_graphics_family = true;
				graphics_family = j;
			}

			VkBool32 supports_surface;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], j, surface, &supports_surface);
			if (supports_surface) {
				found_present_family = true;
				present_family = j;
			}
		}

		if (found_graphics_family && found_present_family) {
			physical_device = physical_devices[i];
			break;
		}

		found_graphics_family = false;
		found_present_family = false;
	}

	if (!found_all_extensions) {
		printf("No device that supports all required extensions found\n");
		return;
	}

	if (!found_graphics_family) {
		printf("No queue family that supports graphics and compute found\n");
		return;
	}

	if (!found_present_family) {
		printf("No queue family that supports presenting to surface found\n");
		return;
	}

	// Create logical device
	f32 queue_priority = 1.0f;
	VkDeviceQueueCreateInfo graphics_queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = graphics_family,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority
	};

	VkDeviceQueueCreateInfo present_queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = present_family,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority
	};

	VkDeviceQueueCreateInfo queue_create_infos[] = { graphics_queue_create_info, present_queue_create_info };

	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pQueueCreateInfos = queue_create_infos,
		.queueCreateInfoCount = 2,
		.ppEnabledLayerNames = enabled_layers,
		.enabledLayerCount = enabled_layers_count,
		.ppEnabledExtensionNames = device_extensions,
		.enabledExtensionCount = device_extension_count
	};

	VkDevice device;
	if (vkCreateDevice(physical_device, &device_create_info, NULL, &device) != VK_SUCCESS) {
		printf("Failed to create logical device\n");
		return;
	}

	// Query surface capabilities
	u32 surface_format_count;
	VkResult res;
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, NULL);
	VkSurfaceFormatKHR surface_formats[surface_format_count];
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats);

	u32 present_mode_count;
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);

	// Create swapchain
	VkSurfaceFormatKHR chosen_format;
	bool found_desired_format = false;
	FOR(surface_format_index, surface_format_count) {
		VkSurfaceFormatKHR format = surface_formats[surface_format_index];
		if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			chosen_format = format;
			found_desired_format = true;
			break;
		}
	}

	if (!found_desired_format) {
		chosen_format = surface_formats[0];
		printf("Desired surface format not found, falling back to another option\n");
	}

	VkPresentModeKHR chosen_present_mode;
	bool found_desired_present_mode = false;
	FOR(present_mode_index, present_mode_count) {
		if (present_modes[present_mode_index] == VK_PRESENT_MODE_MAILBOX_KHR) {
			chosen_present_mode = present_modes[present_mode_index];
			found_desired_present_mode = true;
			break;
		}
	}

	if (!found_desired_present_mode) {
		chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
		printf("Desired present mode not found, falling back to FIFO\n");
	}

	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

	VkExtent2D swap_extent = surface_capabilities.currentExtent;
	if (surface_capabilities.currentExtent.width == UINT32_MAX) {
		const VkExtent2D min_extent = surface_capabilities.minImageExtent;
		const VkExtent2D max_extent = surface_capabilities.maxImageExtent;
		swap_extent.width = u32_clamp(min_extent.width, window_width, max_extent.width);
		swap_extent.height = u32_clamp(min_extent.height, window_height, max_extent.height);
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = surface_capabilities.minImageCount,
		.imageFormat = chosen_format.format,
		.imageColorSpace = chosen_format.colorSpace,
		.imageExtent = swap_extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = 0,
		.preTransform = surface_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = chosen_present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	if (graphics_family != present_family) {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = 2;
		u32 family_indices[] = { graphics_family, present_family };
		swapchain_create_info.pQueueFamilyIndices = family_indices;
	} else {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkSwapchainKHR swapchain;
	if (vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain) != VK_SUCCESS) {
		printf("Was unable to create swap chain\n");
		return;
	}

	// Get queues
	VkQueue present_queue;
	VkQueue graphics_queue;
	vkGetDeviceQueue(device, present_family, 0, &present_queue);
	vkGetDeviceQueue(device, graphics_family, 0, &graphics_queue);
}
