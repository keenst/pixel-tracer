#include "vulkan.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data) {
	printf("%s\n", callback_data->pMessage);
	return VK_FALSE;
}

VkCommandBuffer begin_temp_command_buffer(VulkanState* vulkan_state) {
	VkCommandBufferAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = vulkan_state->command_pool,
		.commandBufferCount = 1
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(vulkan_state->device, &allocate_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void end_temp_command_buffer(VulkanState* vulkan_state, VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer
	};

	vkQueueSubmit(vulkan_state->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkan_state->graphics_queue);

	vkFreeCommandBuffers(vulkan_state->device, vulkan_state->command_pool, 1, &command_buffer);
}

void transition_image_layout(VulkanState* vulkan_state, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
	VkCommandBuffer temp_command_buffer = begin_temp_command_buffer(vulkan_state);

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		assert(!"Unsupported layout transition");
	}

	vkCmdPipelineBarrier(
			temp_command_buffer,
			source_stage, destination_stage,
			0,
			0, NULL,
			0, NULL,
			1, &barrier);

	end_temp_command_buffer(vulkan_state, temp_command_buffer);
}

uint32 find_vulkan_memory_type(
		VulkanState* vulkan_state,
		VkMemoryRequirements memory_requirements,
		VkMemoryPropertyFlags property_flags)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(vulkan_state->physical_device, &memory_properties);

	uint32 memory_type;
	bool found_memory_type = false;
	FOR(i, memory_properties.memoryTypeCount) {
		if (memory_requirements.memoryTypeBits & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
			memory_type = i;
			found_memory_type = true;
		}
	}

	assert(found_memory_type && "Failed to find suitable memory type");
	return memory_type;
}

void create_buffer(
		VulkanState* vulkan_state,
		VkDeviceSize size,
		VkBufferUsageFlags usage_flags,
		VkMemoryPropertyFlags property_flags,
		VkBuffer* buffer,
		VkDeviceMemory* buffer_memory)
{
	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage_flags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	if (vkCreateBuffer(vulkan_state->device, &buffer_create_info, NULL, buffer) != VK_SUCCESS) {
		assert(!"Failed to create buffer");
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(vulkan_state->device, *buffer, &memory_requirements);

	uint32 memory_type = find_vulkan_memory_type(vulkan_state, memory_requirements, property_flags);

	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = memory_type,
	};

	if (vkAllocateMemory(vulkan_state->device, &allocate_info, NULL, buffer_memory) != VK_SUCCESS) {
		assert(!"Failed to allocate buffer memory");
	}

	vkBindBufferMemory(vulkan_state->device, *buffer, *buffer_memory, 0);
}

void create_swapchain(VulkanState* vulkan_state, uint32 window_width, uint32 window_height) {
	vkDeviceWaitIdle(vulkan_state->device);

	FOR(i, vulkan_state->image_view_count) {
		vkDestroyFramebuffer(vulkan_state->device, vulkan_state->framebuffers[i], NULL);
		vkDestroyImageView(vulkan_state->device, vulkan_state->image_views[i], NULL);
	}

	vkDestroySwapchainKHR(vulkan_state->device, vulkan_state->swapchain, NULL);

	// Create swapchain
	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
			vulkan_state->physical_device,
			vulkan_state->surface,
			&surface_capabilities);

	vulkan_state->swap_extent = surface_capabilities.currentExtent;
	if (surface_capabilities.currentExtent.width == UINT32_MAX) {
		const VkExtent2D min_extent = surface_capabilities.minImageExtent;
		const VkExtent2D max_extent = surface_capabilities.maxImageExtent;
		vulkan_state->swap_extent.width = uint32_clamp(min_extent.width, window_width, max_extent.width);
		vulkan_state->swap_extent.height = uint32_clamp(min_extent.height, window_height, max_extent.height);
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = vulkan_state->surface,
		.minImageCount = surface_capabilities.minImageCount,
		.imageFormat = vulkan_state->surface_format.format,
		.imageColorSpace = vulkan_state->surface_format.colorSpace,
		.imageExtent = vulkan_state->swap_extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = 0,
		.preTransform = surface_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = vulkan_state->present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	if (vulkan_state->graphics_family != vulkan_state->present_family) {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = 2;
		uint32 family_indices[] = { vulkan_state->graphics_family, vulkan_state->present_family };
		swapchain_create_info.pQueueFamilyIndices = family_indices;
	} else {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkSwapchainKHR swapchain;
	VK_ASSERT(vkCreateSwapchainKHR(vulkan_state->device, &swapchain_create_info, NULL, &swapchain));
	vulkan_state->swapchain = swapchain;

	vkGetSwapchainImagesKHR(
			vulkan_state->device,
			vulkan_state->swapchain,
			&vulkan_state->image_view_count,
			NULL);

	VkImage swapchain_images[vulkan_state->image_view_count];
	vkGetSwapchainImagesKHR(
			vulkan_state->device,
			vulkan_state->swapchain,
			&vulkan_state->image_view_count,
			swapchain_images);

	// Create image views
	if (vulkan_state->image_views) {
		free(vulkan_state->image_views);
	}

	vulkan_state->image_views = malloc(vulkan_state->image_view_count * sizeof(VkImageView));

	FOR(image_index, vulkan_state->image_view_count) {
		VkImageViewCreateInfo image_view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapchain_images[image_index],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = vulkan_state->surface_format.format,
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

		VK_ASSERT(vkCreateImageView(
				vulkan_state->device,
				&image_view_create_info,
				NULL,
				&vulkan_state->image_views[image_index]));
	}

	// Create framebuffers
	if (vulkan_state->framebuffers) {
		free(vulkan_state->framebuffers);
	}

	vulkan_state->framebuffers = malloc(vulkan_state->image_view_count * sizeof(VkFramebuffer));
	FOR(image_view_index, vulkan_state->image_view_count) {
		VkImageView attachments[] = { vulkan_state->image_views[image_view_index] };

		VkFramebufferCreateInfo framebuffer_create_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = vulkan_state->render_pass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = vulkan_state->swap_extent.width,
			.height = vulkan_state->swap_extent.height,
			.layers = 1
		};

		VK_ASSERT(vkCreateFramebuffer(
				vulkan_state->device,
				&framebuffer_create_info,
				NULL,
				&vulkan_state->framebuffers[image_view_index]));
	}

	return;
}

VulkanState setup_renderer(
		VulkanPlatformData vulkan_platform_data,
		uint32 window_width,
		uint32 window_height)
{
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = vulkan_platform_data.func_vkGetInstanceProcAddr;
	PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = vulkan_platform_data.func_vkEnumerateInstanceLayerProperties;

	VulkanState vulkan_state = {};
	vulkan_state.surface = vulkan_platform_data.surface;

	// Create debug callback
	VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = vulkan_debug_callback
	};

	vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_platform_data.instance, "vkCreateDebugUtilsMessengerEXT");
	if (vkCreateDebugUtilsMessengerEXT) {
		VkDebugUtilsMessengerEXT debug_messenger;
		vkCreateDebugUtilsMessengerEXT(vulkan_platform_data.instance, &messenger_create_info, NULL, &debug_messenger);
	}

	// Print available layers
	uint32 layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);
	VkLayerProperties available_layers[layer_count];
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	// Look for a suitable physical device
	uint32 physical_device_count;
	vkEnumeratePhysicalDevices(vulkan_platform_data.instance, &physical_device_count, NULL);
	VkPhysicalDevice physical_devices[physical_device_count];
	vkEnumeratePhysicalDevices(vulkan_platform_data.instance, &physical_device_count, physical_devices);

	bool found_graphics_family = false;
	bool found_present_family = false;
	bool found_all_extensions = true;
	for (uint32 i = 0; i < physical_device_count; i++) {
		// Check extensions
		const uint32 max_available_extension_count = 512;
		uint32 available_extension_count = max_available_extension_count;
		VkExtensionProperties available_extensions[max_available_extension_count];
		vkEnumerateDeviceExtensionProperties(
				physical_devices[i],
				NULL,
				&available_extension_count,
				available_extensions);

		assert(available_extension_count <= max_available_extension_count);

		FOR(required_extension_index, vulkan_platform_data.device_extension_count) {
			bool found_extension = false;

			FOR(available_extension_index, available_extension_count) {
				if (strcmp(
					available_extensions[available_extension_index].extensionName,
					vulkan_platform_data.device_extensions[required_extension_index]) == 0)
				{
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
		uint32 queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(
				physical_devices[i],
				&queue_family_count,
				NULL);

		VkQueueFamilyProperties queue_families[queue_family_count];
		vkGetPhysicalDeviceQueueFamilyProperties(
				physical_devices[i],
				&queue_family_count,
				queue_families);

		for (uint32 j = 0; j < queue_family_count; j++) {
			if (queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
				queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
				found_graphics_family = true;
				vulkan_state.graphics_family = j;
			}

			VkBool32 supports_surface;
			vkGetPhysicalDeviceSurfaceSupportKHR(
					physical_devices[i],
					j,
					vulkan_platform_data.surface,
					&supports_surface);

			if (supports_surface) {
				found_present_family = true;
				vulkan_state.present_family = j;
			}
		}

		if (found_graphics_family && found_present_family) {
			vulkan_state.physical_device = physical_devices[i];
			break;
		}

		found_graphics_family = false;
		found_present_family = false;
	}

	if (!found_all_extensions) {
		assert(!"No device that supports all required extensions found");
	}

	if (!found_graphics_family) {
		assert(!"No queue family that supports graphics and compute found");
	}

	if (!found_present_family) {
		assert(!"No queue family that supports presenting to surface found");
	}

	// Query surface capabilities
	uint32 surface_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(
			vulkan_state.physical_device,
			vulkan_platform_data.surface,
			&surface_format_count,
			NULL);

	VkSurfaceFormatKHR surface_formats[surface_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(
			vulkan_state.physical_device,
			vulkan_platform_data.surface,
			&surface_format_count,
			surface_formats);

	uint32 present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
			vulkan_state.physical_device,
			vulkan_platform_data.surface,
			&present_mode_count,
			NULL);

	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(
			vulkan_state.physical_device,
			vulkan_platform_data.surface,
			&present_mode_count,
			present_modes);

	// Find desired modes and formats
	bool found_desired_format = false;
	FOR(surface_format_index, surface_format_count) {
		VkSurfaceFormatKHR format = surface_formats[surface_format_index];
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
			vulkan_state.surface_format = format;
			found_desired_format = true;
			break;
		}
	}

	if (!found_desired_format) {
		vulkan_state.surface_format = surface_formats[0];
		printf("Desired surface format not found, falling back to another option\n");
	}

	bool found_desired_present_mode = false;
	FOR(present_mode_index, present_mode_count) {
		if (present_modes[present_mode_index] == VK_PRESENT_MODE_MAILBOX_KHR) {
			vulkan_state.present_mode = present_modes[present_mode_index];
			found_desired_present_mode = true;
			break;
		}
	}

	if (!found_desired_present_mode) {
		vulkan_state.present_mode = VK_PRESENT_MODE_FIFO_KHR;
		printf("Desired present mode not found, falling back to FIFO\n");
	}

	// Create logical device
	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo graphics_queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = vulkan_state.graphics_family,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority
	};

	VkDeviceQueueCreateInfo present_queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = vulkan_state.present_family,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority
	};

	VkDeviceQueueCreateInfo queue_create_infos[] = { graphics_queue_create_info, present_queue_create_info };

	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pQueueCreateInfos = queue_create_infos,
		.queueCreateInfoCount = 2,
		.ppEnabledLayerNames = (const char* const*)vulkan_platform_data.enabled_layers,
		.enabledLayerCount = vulkan_platform_data.enabled_layer_count,
		.ppEnabledExtensionNames = (const char* const*)vulkan_platform_data.device_extensions,
		.enabledExtensionCount = vulkan_platform_data.device_extension_count
	};

	VK_ASSERT(vkCreateDevice(
			vulkan_state.physical_device,
			&device_create_info,
			NULL,
			&vulkan_state.device));

	// Get queues
	vkGetDeviceQueue(vulkan_state.device, vulkan_state.present_family, 0, &vulkan_state.present_queue);
	vkGetDeviceQueue(vulkan_state.device, vulkan_state.graphics_family, 0, &vulkan_state.graphics_queue);

	/*============*/
	/*  Commands  */
	/*============*/

	VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vulkan_state.graphics_family
	};

	VK_ASSERT(vkCreateCommandPool(
			vulkan_state.device,
			&command_pool_create_info,
			NULL,
			&vulkan_state.command_pool));

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vulkan_state.command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = MAX_FRAMES_IN_FLIGHT
	};

	VK_ASSERT(vkAllocateCommandBuffers(
			vulkan_state.device,
			&command_buffer_allocate_info,
			vulkan_state.command_buffers));

	vulkan_state.command_buffer_begin_info = (VkCommandBufferBeginInfo){
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};

	VK_ASSERT(vkAllocateCommandBuffers(
			vulkan_state.device,
			&command_buffer_allocate_info,
			vulkan_state.compute_command_buffers));

	/*=========*/
	/*  Image  */
	/*=========*/

	// Create image
	VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = 320,
		.extent.height = 180,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.samples = VK_SAMPLE_COUNT_1_BIT
	};

	VK_ASSERT(vkCreateImage(
			vulkan_state.device,
			&image_create_info,
			NULL,
			&vulkan_state.render_texture_image));

	// Allocate image memory
	VkMemoryRequirements texture_memory_requirements;
	vkGetImageMemoryRequirements(
			vulkan_state.device,
			vulkan_state.render_texture_image,
			&texture_memory_requirements);

	VkMemoryAllocateInfo texture_memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = texture_memory_requirements.size,
		.memoryTypeIndex = find_vulkan_memory_type(&vulkan_state, texture_memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	VkDeviceMemory render_texture_image_memory;
	VK_ASSERT(vkAllocateMemory(
			vulkan_state.device,
			&texture_memory_allocate_info,
			NULL,
			&render_texture_image_memory));

	vkBindImageMemory(
			vulkan_state.device, 
			vulkan_state.render_texture_image,
			render_texture_image_memory,
			0);

	transition_image_layout(
			&vulkan_state,
			vulkan_state.render_texture_image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create image view
	VkImageViewCreateInfo render_texture_image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = vulkan_state.render_texture_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	VkImageView render_texture_image_view;
	VK_ASSERT(vkCreateImageView(
			vulkan_state.device,
			&render_texture_image_view_create_info,
			NULL,
			&render_texture_image_view));

	// Create texture sampler
	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.anisotropyEnable = VK_FALSE,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_TRUE,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST
	};

	VkSampler texture_sampler;
	VK_ASSERT(vkCreateSampler(vulkan_state.device, &sampler_info, NULL, &texture_sampler));

	/*===================*/
	/*  DESCRIPTOR SETS  */
	/*===================*/

	// Create descriptor pool
	VkDescriptorPoolSize uniform_pool_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = MAX_FRAMES_IN_FLIGHT
	};

	VkDescriptorPoolSize sampler_pool_size = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = MAX_FRAMES_IN_FLIGHT
	};

	VkDescriptorPoolSize compute_pool_size = {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = MAX_FRAMES_IN_FLIGHT
	};

	VkDescriptorPoolSize storage_buffer_pool_size = {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = MAX_FRAMES_IN_FLIGHT
	};

	VkDescriptorPoolSize pool_sizes[] = {
		uniform_pool_size,
		sampler_pool_size,
		compute_pool_size,
		storage_buffer_pool_size,
		storage_buffer_pool_size
	};

	VkDescriptorPoolCreateInfo pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = 5,
		.pPoolSizes = pool_sizes,
		.maxSets = MAX_FRAMES_IN_FLIGHT * 2
	};

	VkDescriptorPool descriptor_pool;
	VK_ASSERT(vkCreateDescriptorPool(
			vulkan_state.device,
			&pool_create_info,
			NULL,
			&descriptor_pool));

	// Create descriptor set layouts
	VkDescriptorSetLayoutBinding sampler_layout_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &sampler_layout_binding
	};

	VkDescriptorSetLayout descriptor_set_layout;
	VK_ASSERT(vkCreateDescriptorSetLayout(
			vulkan_state.device,
			&descriptor_set_layout_create_info,
			NULL,
			&descriptor_set_layout));

	VkDescriptorSetLayout descriptor_set_layouts[MAX_FRAMES_IN_FLIGHT];
	FOR(i, MAX_FRAMES_IN_FLIGHT) {
		descriptor_set_layouts[i] = descriptor_set_layout;
	}

	// Allocate descriptor sets
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptor_pool,
		.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
		.pSetLayouts = descriptor_set_layouts
	};

	VK_ASSERT(vkAllocateDescriptorSets(
			vulkan_state.device,
			&descriptor_set_allocate_info,
			vulkan_state.descriptor_sets));

	// Update descriptor sets
	FOR(i, MAX_FRAMES_IN_FLIGHT) {
		VkDescriptorImageInfo image_info = {
			.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, // NOTE: Could be bad? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL is suggested by the tutorial, but I need to write to it as well
			.imageView = render_texture_image_view,
			.sampler = texture_sampler
		};

		VkWriteDescriptorSet write_descriptors[1];

		write_descriptors[0] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vulkan_state.descriptor_sets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.pImageInfo = &image_info
		};

		vkUpdateDescriptorSets(vulkan_state.device, 1, write_descriptors, 0, NULL);
	}

	/*=====================*/
	/*  GRAPHICS PIPELINE  */
	/*=====================*/

	// Create shader module
	uint32 shader_code_size;
	char* shader_code = platform_read_file("data/shaders/triangle.spv", &shader_code_size);

	VkShaderModuleCreateInfo shader_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shader_code_size,
		.pCode = (uint32*)shader_code
	};

	VkShaderModule shader_module;
	vkCreateShaderModule(vulkan_state.device, &shader_create_info, NULL, &shader_module);

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

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		vertex_shader_stage_create_info,
		fragment_shader_stage_create_info
	};

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptor_set_layout
	};

	VK_ASSERT(vkCreatePipelineLayout(
			vulkan_state.device,
			&pipeline_layout_create_info,
			NULL,
			&vulkan_state.graphics_pipeline_layout));

	// Create render pass
	VkAttachmentDescription color_attachment = {
		.format = vulkan_state.surface_format.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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

	// Create graphics pipeline
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

	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
			vulkan_state.physical_device,
			vulkan_platform_data.surface,
			&surface_capabilities);

	vulkan_state.swap_extent = surface_capabilities.currentExtent;
	if (surface_capabilities.currentExtent.width == UINT32_MAX) {
		const VkExtent2D min_extent = surface_capabilities.minImageExtent;
		const VkExtent2D max_extent = surface_capabilities.maxImageExtent;
		vulkan_state.swap_extent.width = uint32_clamp(min_extent.width, window_width, max_extent.width);
		vulkan_state.swap_extent.height = uint32_clamp(min_extent.height, window_height, max_extent.height);
	}

	vulkan_state.viewport = (VkViewport){
		.x = 0,
		.y = 0,
		.width = (float)vulkan_state.swap_extent.width,
		.height = (float)vulkan_state.swap_extent.height,
		.minDepth = 0,
		.maxDepth = 1
	};

	vulkan_state.scissor = (VkRect2D){
		.offset = { 0, 0 },
		.extent = vulkan_state.swap_extent
	};

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = NULL,
		.scissorCount = 1,
		.pScissors = NULL
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
		.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
	};

	VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment
	};

	VK_ASSERT(vkCreateRenderPass(
			vulkan_state.device,
			&render_pass_create_info,
			NULL,
			&vulkan_state.render_pass));

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
		.layout = vulkan_state.graphics_pipeline_layout,
		.renderPass = vulkan_state.render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VK_ASSERT(vkCreateGraphicsPipelines(
			vulkan_state.device,
			VK_NULL_HANDLE,
			1, &pipeline_create_info,
			NULL,
			&vulkan_state.graphics_pipeline));

	/*====================*/
	/*  COMPUTE PIPELINE  */
	/*====================*/

	// Shader
	uint32 compute_shader_code_size;
	char* compute_shader_code = platform_read_file("data/shaders/compute.spv", &compute_shader_code_size);

	VkShaderModuleCreateInfo compute_shader_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = compute_shader_code_size,
		.pCode = (uint32*)compute_shader_code
	};

	VkShaderModule compute_shader_module;
	vkCreateShaderModule(vulkan_state.device, &compute_shader_create_info, NULL, &compute_shader_module);

	VkPipelineShaderStageCreateInfo compute_shader_stage_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = compute_shader_module,
		.pName = "main"
	};

	// Descriptor sets
	VkDescriptorSetLayoutBinding rw_texture_layout_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	};

	VkDescriptorSetLayoutBinding uniform_buffer_layout_binding = {
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	};

	VkDescriptorSetLayoutBinding triangle_buffer_layout_binding = {
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	};

	VkDescriptorSetLayoutBinding bvh_buffer_layout_binding = {
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	};

	VkDescriptorSetLayoutBinding compute_descriptor_set_layout_bindings[] = {
		rw_texture_layout_binding,
		uniform_buffer_layout_binding,
		triangle_buffer_layout_binding,
		bvh_buffer_layout_binding
	};

	VkDescriptorSetLayoutCreateInfo compute_descriptor_set_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 4,
		.pBindings = compute_descriptor_set_layout_bindings
	};

	VkDescriptorSetLayout compute_descriptor_set_layout;
	VK_ASSERT(vkCreateDescriptorSetLayout(
			vulkan_state.device,
			&compute_descriptor_set_layout_create_info,
			NULL,
			&compute_descriptor_set_layout));

	VkDescriptorSetLayout compute_descriptor_set_layouts[MAX_FRAMES_IN_FLIGHT];
	FOR(i, MAX_FRAMES_IN_FLIGHT) {
		compute_descriptor_set_layouts[i] = compute_descriptor_set_layout;
	}

	VkDescriptorSetAllocateInfo compute_descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptor_pool,
		.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
		.pSetLayouts = compute_descriptor_set_layouts
	};

	VK_ASSERT(vkAllocateDescriptorSets(
			vulkan_state.device,
			&compute_descriptor_set_allocate_info,
			vulkan_state.compute_descriptor_sets));

	// Allocate and map uniform buffers
	FOR(i, MAX_FRAMES_IN_FLIGHT) {
		create_buffer(
				&vulkan_state,
				sizeof(RendererState),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&vulkan_state.renderer_state_buffers[i],
				&vulkan_state.renderer_state_buffers_memory[i]);

		vkMapMemory(
				vulkan_state.device,
				vulkan_state.renderer_state_buffers_memory[i],
				0, sizeof(RendererState),
				0, &vulkan_state.renderer_state_buffers_mapped[i]);
	}

	create_buffer(
			&vulkan_state,
			sizeof(Triangle) * MAX_TRIANGLE_BUFFER_COUNT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vulkan_state.triangle_buffer,
			&vulkan_state.triangle_buffer_memory);

	vkMapMemory(
			vulkan_state.device,
			vulkan_state.triangle_buffer_memory,
			0, sizeof(Triangle) * MAX_TRIANGLE_BUFFER_COUNT,
			0, &vulkan_state.triangle_buffer_mapped);

	create_buffer(
			&vulkan_state,
			sizeof(BVHNodeFlat) * MAX_BVH_BUFFER_COUNT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vulkan_state.bvh_buffer,
			&vulkan_state.bvh_buffer_memory);

	vkMapMemory(
			vulkan_state.device,
			vulkan_state.bvh_buffer_memory,
			0, sizeof(BVHNodeFlat) * MAX_BVH_BUFFER_COUNT,
			0, &vulkan_state.bvh_buffer_mapped);

	// Update descriptor sets
	FOR(i, MAX_FRAMES_IN_FLIGHT) {
		VkDescriptorBufferInfo uniform_buffer_info = {
			.buffer = vulkan_state.renderer_state_buffers[i],
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};

		VkDescriptorImageInfo storage_buffer_info = {
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
			.imageView = render_texture_image_view,
			.sampler = texture_sampler
		};

		VkDescriptorBufferInfo triangle_buffer_info = {
			.buffer = vulkan_state.triangle_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};

		VkDescriptorBufferInfo bvh_buffer_info = {
			.buffer = vulkan_state.bvh_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};

		VkWriteDescriptorSet write_descriptors[4];

		write_descriptors[0] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vulkan_state.compute_descriptor_sets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.pImageInfo = &storage_buffer_info
		};

		write_descriptors[1] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vulkan_state.compute_descriptor_sets[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo = &uniform_buffer_info
		};

		write_descriptors[2] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vulkan_state.compute_descriptor_sets[i],
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo = &triangle_buffer_info
		};

		write_descriptors[3] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vulkan_state.compute_descriptor_sets[i],
			.dstBinding = 3,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo = &bvh_buffer_info
		};

		vkUpdateDescriptorSets(vulkan_state.device, 4, write_descriptors, 0, NULL);
	}

	// Pipeline
	VkPipelineLayoutCreateInfo compute_pipeline_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &compute_descriptor_set_layout
	};

	VK_ASSERT(vkCreatePipelineLayout(
			vulkan_state.device,
			&compute_pipeline_layout_create_info,
			NULL,
			&vulkan_state.compute_pipeline_layout));

	VkComputePipelineCreateInfo compute_pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.layout = vulkan_state.compute_pipeline_layout,
		.stage = compute_shader_stage_create_info
	};

	VK_ASSERT(vkCreateComputePipelines(
			vulkan_state.device,
			VK_NULL_HANDLE,
			1, &compute_pipeline_create_info,
			NULL,
			&vulkan_state.compute_pipeline));

	/*============*/
	/*  Finalize  */
	/*============*/

	// Create sync primitives
	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	FOR(i, MAX_FRAMES_IN_FLIGHT) {
		// Graphics
		VK_ASSERT(vkCreateSemaphore(
				vulkan_state.device,
				&semaphore_create_info,
				NULL,
				&vulkan_state.image_available_semaphores[i]));

		VK_ASSERT(vkCreateSemaphore(
				vulkan_state.device,
				&semaphore_create_info,
				NULL,
				&vulkan_state.render_finished_semaphores[i]));

		VK_ASSERT(vkCreateFence(
				vulkan_state.device,
				&fence_create_info,
				NULL,
				&vulkan_state.in_flight_fences[i]));

		// Compute
		VK_ASSERT(vkCreateSemaphore(
				vulkan_state.device,
				&semaphore_create_info,
				NULL,
				&vulkan_state.compute_finished_semaphores[i]));

		VK_ASSERT(vkCreateFence(
				vulkan_state.device,
				&fence_create_info,
				NULL,
				&vulkan_state.compute_in_flight_fences[i]));
	}

	// Start a render pass
	create_swapchain(&vulkan_state, window_width, window_height);

	VkClearValue clear_color = { 0, 0, 0, 1 };

	vulkan_state.render_pass_begin_info = (VkRenderPassBeginInfo){
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = vulkan_state.render_pass,
		.framebuffer = vulkan_state.framebuffers[0],
		.renderArea.offset = { 0, 0 },
		.renderArea.extent = vulkan_state.swap_extent,
		.clearValueCount = 1,
		.pClearValues = &clear_color
	};

	vulkan_state.is_initialized = true;
	return vulkan_state;
}

void load_vulkan(VulkanPlatformData vulkan_platform_data) {
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = vulkan_platform_data.func_vkGetInstanceProcAddr;

#define VK_LOAD(func) \
	func = (PFN_##func)vkGetInstanceProcAddr(vulkan_platform_data.instance, #func); \
	assert(func);

	VK_LOAD(vkEnumeratePhysicalDevices);
	VK_LOAD(vkGetPhysicalDeviceProperties);
	VK_LOAD(vkGetPhysicalDeviceFeatures);
	VK_LOAD(vkGetPhysicalDeviceFeatures2);
	VK_LOAD(vkGetPhysicalDeviceQueueFamilyProperties);
	VK_LOAD(vkCreateDevice);
	VK_LOAD(vkGetDeviceQueue);
	VK_LOAD(vkGetPhysicalDeviceSurfaceSupportKHR);
	VK_LOAD(vkEnumerateDeviceExtensionProperties);
	VK_LOAD(vkGetPhysicalDeviceSurfaceFormatsKHR);
	VK_LOAD(vkGetPhysicalDeviceSurfacePresentModesKHR);
	VK_LOAD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
	VK_LOAD(vkCreateSwapchainKHR);
	VK_LOAD(vkGetSwapchainImagesKHR);
	VK_LOAD(vkCreateImageView);
	VK_LOAD(vkCreateShaderModule);
	VK_LOAD(vkCreatePipelineLayout);
	VK_LOAD(vkCreateRenderPass);
	VK_LOAD(vkCreateGraphicsPipelines);
	VK_LOAD(vkCreateFramebuffer);
	VK_LOAD(vkCreateCommandPool);
	VK_LOAD(vkAllocateCommandBuffers);
	VK_LOAD(vkBeginCommandBuffer);
	VK_LOAD(vkCmdBeginRenderPass);
	VK_LOAD(vkCmdBindPipeline);
	VK_LOAD(vkCmdSetViewport);
	VK_LOAD(vkCmdSetScissor);
	VK_LOAD(vkCmdDraw);
	VK_LOAD(vkCmdEndRenderPass);
	VK_LOAD(vkEndCommandBuffer);
	VK_LOAD(vkCreateSemaphore);
	VK_LOAD(vkCreateFence);
	VK_LOAD(vkWaitForFences);
	VK_LOAD(vkResetFences);
	VK_LOAD(vkAcquireNextImageKHR);
	VK_LOAD(vkResetCommandBuffer);
	VK_LOAD(vkQueueSubmit);
	VK_LOAD(vkQueuePresentKHR);
	VK_LOAD(vkDeviceWaitIdle);
	VK_LOAD(vkDestroySwapchainKHR);
	VK_LOAD(vkDestroyFramebuffer);
	VK_LOAD(vkDestroyImageView);
	VK_LOAD(vkCreateBuffer);
	VK_LOAD(vkGetBufferMemoryRequirements);
	VK_LOAD(vkGetPhysicalDeviceMemoryProperties);
	VK_LOAD(vkAllocateMemory);
	VK_LOAD(vkBindBufferMemory);
	VK_LOAD(vkMapMemory);
	VK_LOAD(vkUnmapMemory);
	VK_LOAD(vkCreateImage);
	VK_LOAD(vkGetImageMemoryRequirements);
	VK_LOAD(vkBindImageMemory);
	VK_LOAD(vkQueueWaitIdle);
	VK_LOAD(vkFreeCommandBuffers);
	VK_LOAD(vkCmdPipelineBarrier);
	VK_LOAD(vkCmdCopyBufferToImage);
	VK_LOAD(vkCreateSampler);
	VK_LOAD(vkCreateDescriptorSetLayout);
	VK_LOAD(vkCreateDescriptorPool);
	VK_LOAD(vkAllocateDescriptorSets);
	VK_LOAD(vkUpdateDescriptorSets);
	VK_LOAD(vkCmdBindDescriptorSets);
	VK_LOAD(vkCreateComputePipelines);
	VK_LOAD(vkCmdDispatch);
	VK_LOAD(vkCmdClearColorImage);
#undef VK_LOAD
}
