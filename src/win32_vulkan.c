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
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
PFN_vkCreateImageView vkCreateImageView;
PFN_vkCreateShaderModule vkCreateShaderModule;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
PFN_vkCreateRenderPass vkCreateRenderPass;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
PFN_vkCreateFramebuffer vkCreateFramebuffer;
PFN_vkCreateCommandPool vkCreateCommandPool;
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
PFN_vkCmdBindPipeline vkCmdBindPipeline;
PFN_vkCmdSetViewport vkCmdSetViewport;
PFN_vkCmdSetScissor vkCmdSetScissor;
PFN_vkCmdDraw vkCmdDraw;
PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
PFN_vkEndCommandBuffer vkEndCommandBuffer;
PFN_vkCreateSemaphore vkCreateSemaphore;
PFN_vkCreateFence vkCreateFence;
PFN_vkWaitForFences vkWaitForFences;
PFN_vkResetFences vkResetFences;
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
PFN_vkResetCommandBuffer vkResetCommandBuffer;
PFN_vkQueueSubmit vkQueueSubmit;
PFN_vkQueuePresentKHR vkQueuePresentKHR;

enum { MAX_FRAMES_IN_FLIGHT = 2 };

typedef struct {
	VkDevice device;
	VkSwapchainKHR swapchain;
	VkFramebuffer* framebuffers;
	VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
	VkPipeline graphics_pipeline;
	VkQueue graphics_queue;
	VkQueue present_queue;

	VkViewport viewport;
	VkRect2D scissor;

	VkCommandBufferBeginInfo command_buffer_begin_info;
	VkRenderPassBeginInfo render_pass_begin_info;

	VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
} VulkanState;

u32 u32_clamp(u32 min, u32 value, u32 max) {
	if (value > max) {
		return max;
	}

	if (value < min) {
		return min;
	}

	return value;
}

VulkanState win32_init_vulkan(HWND window, HINSTANCE instance, u32 window_width, u32 window_height) {
	// Load functions
	HMODULE vulkan_dll = LoadLibrary("vulkan-1");
	if (vulkan_dll == NULL) {
		printf("Failed to load vulkan dll\n");
		return (VulkanState){};
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
	vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)GetProcAddress(vulkan_dll, "vkGetSwapchainImagesKHR");
	vkCreateImageView = (PFN_vkCreateImageView)GetProcAddress(vulkan_dll, "vkCreateImageView");
	vkCreateShaderModule = (PFN_vkCreateShaderModule)GetProcAddress(vulkan_dll, "vkCreateShaderModule");
	vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)GetProcAddress(vulkan_dll, "vkCreatePipelineLayout");
	vkCreateRenderPass = (PFN_vkCreateRenderPass)GetProcAddress(vulkan_dll, "vkCreateRenderPass");
	vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)GetProcAddress(vulkan_dll, "vkCreateGraphicsPipelines");
	vkCreateFramebuffer = (PFN_vkCreateFramebuffer)GetProcAddress(vulkan_dll, "vkCreateFramebuffer");
	vkCreateCommandPool = (PFN_vkCreateCommandPool)GetProcAddress(vulkan_dll, "vkCreateCommandPool");
	vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)GetProcAddress(vulkan_dll, "vkAllocateCommandBuffers");
	vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)GetProcAddress(vulkan_dll, "vkBeginCommandBuffer");
	vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)GetProcAddress(vulkan_dll, "vkCmdBeginRenderPass");
	vkCmdBindPipeline = (PFN_vkCmdBindPipeline)GetProcAddress(vulkan_dll, "vkCmdBindPipeline");
	vkCmdSetViewport = (PFN_vkCmdSetViewport)GetProcAddress(vulkan_dll, "vkCmdSetViewport");
	vkCmdSetScissor = (PFN_vkCmdSetScissor)GetProcAddress(vulkan_dll, "vkCmdSetScissor");
	vkCmdDraw = (PFN_vkCmdDraw)GetProcAddress(vulkan_dll, "vkCmdDraw");
	vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)GetProcAddress(vulkan_dll, "vkCmdEndRenderPass");
	vkEndCommandBuffer = (PFN_vkEndCommandBuffer)GetProcAddress(vulkan_dll, "vkEndCommandBuffer");
	vkCreateSemaphore = (PFN_vkCreateSemaphore)GetProcAddress(vulkan_dll, "vkCreateSemaphore");
	vkCreateFence = (PFN_vkCreateFence)GetProcAddress(vulkan_dll, "vkCreateFence");
	vkWaitForFences = (PFN_vkWaitForFences)GetProcAddress(vulkan_dll, "vkWaitForFences");
	vkResetFences = (PFN_vkResetFences)GetProcAddress(vulkan_dll, "vkResetFences");
	vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)GetProcAddress(vulkan_dll, "vkAcquireNextImageKHR");
	vkResetCommandBuffer = (PFN_vkResetCommandBuffer)GetProcAddress(vulkan_dll, "vkResetCommandBuffer");
	vkQueueSubmit = (PFN_vkQueueSubmit)GetProcAddress(vulkan_dll, "vkQueueSubmit");
	vkQueuePresentKHR = (PFN_vkQueuePresentKHR)GetProcAddress(vulkan_dll, "vkQueuePresentKHR");

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
		return (VulkanState){};
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
		return (VulkanState){};
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
		return (VulkanState){};
	}

	if (!found_graphics_family) {
		printf("No queue family that supports graphics and compute found\n");
		return (VulkanState){};
	}

	if (!found_present_family) {
		printf("No queue family that supports presenting to surface found\n");
		return (VulkanState){};
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
		return (VulkanState){};
	}

	// Query surface capabilities
	u32 surface_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, NULL);
	VkSurfaceFormatKHR surface_formats[surface_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats);

	u32 present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);

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
		return (VulkanState){};
	}

	u32 image_count;
	vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
	VkImage swapchain_images[image_count];
	vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images);

	// Create image views
	VkImageView swapchain_image_views[image_count];
	u32 swapchain_image_view_count = image_count;

	FOR(image_index, image_count) {
		VkImageViewCreateInfo image_view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapchain_images[image_index],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = chosen_format.format,
			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1
		};

		if (vkCreateImageView(device, &image_view_create_info, NULL, &swapchain_image_views[image_index]) != VK_SUCCESS) {
			printf("Failed to create image view %i\n", image_index);
			return (VulkanState){};
		}
	}

	// Load shaders
	u32 shader_code_size;
	char* shader_code = win32_read_file("data/shaders/triangle.spv", &shader_code_size);

	VkShaderModuleCreateInfo shader_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shader_code_size,
		.pCode = (uint32_t*)shader_code
	};

	VkShaderModule shader_module;
	vkCreateShaderModule(device, &shader_create_info, NULL, &shader_module);

	VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = shader_module,
		.pName = "vertexMain"
	};

	VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = shader_module,
		.pName = "fragmentMain"
	};

	VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_create_info, fragment_shader_stage_create_info };

	// Create pipeline
	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState),
		.pDynamicStates = dynamic_states
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkViewport viewport = {
		.x = 0,
		.y = 0,
		.width = (float)swap_extent.width,
		.height = (float)swap_extent.height,
		.minDepth = 0,
		.maxDepth = 1
	};

	VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = swap_extent
	};

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
	};

	VkPipelineMultisampleStateCreateInfo multisampling_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
	};

	VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment
	};

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	};

	VkPipelineLayout pipeline_layout;
	if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout) != VK_SUCCESS) {
		printf("Failed to create pipeline layout\n");
		return (VulkanState){};
	}

	VkAttachmentDescription color_attachment = {
		.format = chosen_format.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref
	};

	VkSubpassDependency subpass_dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};

	VkRenderPassCreateInfo render_pass_create_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &subpass_dependency
	};

	VkRenderPass render_pass;
	if (vkCreateRenderPass(device, &render_pass_create_info, NULL, &render_pass) != VK_SUCCESS) {
		printf("Failed to create render pass\n");
		return (VulkanState){};
	}

	VkGraphicsPipelineCreateInfo pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shader_stages,
		.pVertexInputState = &vertex_input_state_create_info,
		.pInputAssemblyState = &input_assembly_create_info,
		.pViewportState = &viewport_state_create_info,
		.pRasterizationState = &rasterizer_create_info,
		.pMultisampleState = &multisampling_create_info,
		.pDepthStencilState = NULL,
		.pColorBlendState = &color_blend_create_info,
		.pDynamicState = &dynamic_state_create_info,
		.layout = pipeline_layout,
		.renderPass = render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VkPipeline graphics_pipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &graphics_pipeline) != VK_SUCCESS) {
		printf("Failed to create graphics pipeline\n");
		return (VulkanState){};
	}

	VkFramebuffer* framebuffers = malloc(swapchain_image_view_count * sizeof(VkFramebuffer));
	FOR(image_view_index, swapchain_image_view_count) {
		VkImageView attachments[] = { swapchain_image_views[image_view_index] };

		VkFramebufferCreateInfo framebuffer_create_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = render_pass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = swap_extent.width,
			.height = swap_extent.height,
			.layers = 1
		};

		if (vkCreateFramebuffer(device, &framebuffer_create_info, NULL, &framebuffers[image_view_index]) != VK_SUCCESS) {
			printf("Failed to create framebuffer %i\n", image_view_index);
			return (VulkanState){};
		}
	}

	// Create command pool and command buffer
	VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = graphics_family
	};

	VkCommandPool command_pool;
	if (vkCreateCommandPool(device, &command_pool_create_info, NULL, &command_pool) != VK_SUCCESS) {
		printf("Failed to create command pool\n");
		return (VulkanState){};
	}

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = MAX_FRAMES_IN_FLIGHT
	};

	VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
	if (vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers) != VK_SUCCESS) {
		printf("Failed to allocate command buffers\n");
		return (VulkanState){};
	}

	VkCommandBufferBeginInfo command_buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	// Create sync primitives
	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
	FOR(i, MAX_FRAMES_IN_FLIGHT) {
		if (vkCreateSemaphore(device, &semaphore_create_info, NULL, &image_available_semaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphore_create_info, NULL, &render_finished_semaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fence_create_info, NULL, &in_flight_fences[i]) != VK_SUCCESS) {
			printf("Failed to create sync primitives\n");
			return (VulkanState){};
		}
	}

	// Start a render pass
	VkClearValue clear_color = { 0, 0, 0, 1 };

	VkRenderPassBeginInfo render_pass_begin_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = render_pass,
		.framebuffer = framebuffers[0],
		.renderArea.offset = { 0, 0 },
		.renderArea.extent = swap_extent,
		.clearValueCount = 1,
		.pClearValues = &clear_color
	};

	// Get queues
	VkQueue present_queue;
	VkQueue graphics_queue;
	vkGetDeviceQueue(device, present_family, 0, &present_queue);
	vkGetDeviceQueue(device, graphics_family, 0, &graphics_queue);

	VulkanState output = {
		.device = device,
		.swapchain = swapchain,
		.framebuffers = framebuffers,
		.graphics_pipeline = graphics_pipeline,
		.graphics_queue = graphics_queue,
		.present_queue = present_queue,
		.viewport = viewport,
		.scissor = scissor,
		.command_buffer_begin_info = command_buffer_begin_info,
		.render_pass_begin_info = render_pass_begin_info,
	};

	memcpy(output.command_buffers, command_buffers, sizeof(command_buffers));
	memcpy(output.image_available_semaphores, image_available_semaphores, sizeof(image_available_semaphores));
	memcpy(output.render_finished_semaphores, render_finished_semaphores, sizeof(render_finished_semaphores));
	memcpy(output.in_flight_fences, in_flight_fences, sizeof(in_flight_fences));

	return output;
}
