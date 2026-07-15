VulkanPlatformData win32_init_vulkan(HWND window, HINSTANCE instance) {
	HMODULE vulkan_dll = LoadLibrary("vulkan-1");
	assert(vulkan_dll && "Failed to load vulkan dll");

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkan_dll, "vkGetInstanceProcAddr");
	PFN_vkCreateInstance vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)GetProcAddress(vulkan_dll, "vkCreateWin32SurfaceKHR");

	VulkanPlatformData vulkan_platform_data;
	vulkan_platform_data.func_vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkan_platform_data.func_vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)GetProcAddress(vulkan_dll, "vkEnumerateInstanceLayerProperties");

	// List extensions and layers
	const char* enabled_layers[] = {
		//"VK_LAYER_KHRONOS_validation"
	};

	const char* instance_extensions[] = {
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};

	const char* device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
	};

	const uint32 enabled_layer_count = sizeof(enabled_layers) / sizeof(char*);
	const uint32 instance_extension_count = sizeof(instance_extensions) / sizeof(char*);
	const uint32 device_extension_count = sizeof(device_extensions) / sizeof(char*);

	vulkan_platform_data.enabled_layers = malloc(enabled_layer_count * sizeof(char*));
	vulkan_platform_data.enabled_layer_count = enabled_layer_count;
	FOR(i, enabled_layer_count) {
		uint32 size = strlen(enabled_layers[i]) + 1;
		vulkan_platform_data.enabled_layers[i] = malloc(size);
		memcpy(vulkan_platform_data.enabled_layers[i], enabled_layers[i], size);
	}

	vulkan_platform_data.device_extensions = malloc(device_extension_count * sizeof(char*));
	vulkan_platform_data.device_extension_count = device_extension_count;
	FOR(i, device_extension_count) {
		uint32 size = strlen(device_extensions[i]) + 1;
		vulkan_platform_data.device_extensions[i] = malloc(size);
		memcpy(vulkan_platform_data.device_extensions[i], device_extensions[i], size);
	}

	// Create instance
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Pixel tracer",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_4
	};

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &application_info,
		.enabledExtensionCount = instance_extension_count,
		.ppEnabledExtensionNames = instance_extensions,
		.enabledLayerCount = enabled_layer_count,
		.ppEnabledLayerNames = enabled_layers
	};

	VK_ASSERT(vkCreateInstance(&create_info, NULL, &vulkan_platform_data.instance));

	VkWin32SurfaceCreateInfoKHR surface_create_info = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hwnd = window,
		.hinstance = instance
	};

	VK_ASSERT(vkCreateWin32SurfaceKHR(vulkan_platform_data.instance, &surface_create_info, NULL, &vulkan_platform_data.surface));

	return vulkan_platform_data;
}
