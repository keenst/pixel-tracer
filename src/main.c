#include "main.h"

GameMemory* GAME_MEMORY;
PlatformData* PLATFORM_DATA;
RendererState RENDERER_STATE;

void draw_frame(VulkanState* vulkan_state, uint32 current_frame, RendererState renderer_state) {
	/*======================================*/
	/*               COMPUTE 				*/
	/*======================================*/

	memcpy(vulkan_state->renderer_state_buffers_mapped[current_frame], &RENDERER_STATE, sizeof(RendererState));

	VK_ASSERT(vkWaitForFences(vulkan_state->device, 1, &vulkan_state->compute_in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));
	VK_ASSERT(vkResetFences(vulkan_state->device, 1, &vulkan_state->compute_in_flight_fences[current_frame]));

	VK_ASSERT(vkResetCommandBuffer(vulkan_state->compute_command_buffers[current_frame], 0));
	VK_ASSERT(vkBeginCommandBuffer(vulkan_state->compute_command_buffers[current_frame], &vulkan_state->command_buffer_begin_info));

	vkCmdBindPipeline(
			vulkan_state->compute_command_buffers[current_frame],
			VK_PIPELINE_BIND_POINT_COMPUTE,
			vulkan_state->compute_pipeline);

	vkCmdBindDescriptorSets(
			vulkan_state->compute_command_buffers[current_frame],
			VK_PIPELINE_BIND_POINT_COMPUTE,
			vulkan_state->compute_pipeline_layout,
			0, 1,
			&vulkan_state->compute_descriptor_sets[current_frame],
			0, NULL);

	const uint32 local_size = 8;
	vkCmdDispatch(
			vulkan_state->compute_command_buffers[current_frame],
			320 / local_size + 320 % local_size,
			180 / local_size + 180 % local_size,
			1);

	VK_ASSERT(vkEndCommandBuffer(vulkan_state->compute_command_buffers[current_frame]));

	transition_image_layout(
		vulkan_state,
		vulkan_state->render_texture_image,
		vulkan_state->surface_format.format,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL);

	VkSemaphore compute_signal_semaphores[] = {
		vulkan_state->compute_finished_semaphores[current_frame]
	};

	VkSubmitInfo compute_submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan_state->compute_command_buffers[current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = compute_signal_semaphores
	};

	VK_ASSERT(vkQueueSubmit(
			vulkan_state->graphics_queue,
			1, &compute_submit_info,
			vulkan_state->compute_in_flight_fences[current_frame]));

	/*======================================*/
	/*               GRAPHICS 				*/
	/*======================================*/

	VK_ASSERT(vkWaitForFences(vulkan_state->device, 1, &vulkan_state->in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));
	VK_ASSERT(vkResetFences(vulkan_state->device, 1, &vulkan_state->in_flight_fences[current_frame]));

	uint32 image_index;
	vkAcquireNextImageKHR(
			vulkan_state->device,
			vulkan_state->swapchain,
			UINT64_MAX,
			vulkan_state->image_available_semaphores[current_frame],
			VK_NULL_HANDLE,
			&image_index);

	vulkan_state->render_pass_begin_info.framebuffer = vulkan_state->framebuffers[image_index];

	VK_ASSERT(vkResetCommandBuffer(vulkan_state->command_buffers[current_frame], 0));
	VK_ASSERT(vkBeginCommandBuffer(vulkan_state->command_buffers[current_frame], &vulkan_state->command_buffer_begin_info));

	vkCmdBeginRenderPass(
			vulkan_state->command_buffers[current_frame],
			&vulkan_state->render_pass_begin_info,
			VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(
			vulkan_state->command_buffers[current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vulkan_state->graphics_pipeline);

	vkCmdSetViewport(vulkan_state->command_buffers[current_frame], 0, 1, &vulkan_state->viewport);
	vkCmdSetScissor(vulkan_state->command_buffers[current_frame], 0, 1, &vulkan_state->scissor);

	vkCmdBindDescriptorSets(
			vulkan_state->command_buffers[current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vulkan_state->graphics_pipeline_layout,
			0,
			1, &vulkan_state->descriptor_sets[current_frame],
			0, NULL);

	vkCmdDraw(vulkan_state->command_buffers[current_frame], 6, 1, 0, 0);

	vkCmdEndRenderPass(vulkan_state->command_buffers[current_frame]);

	VK_ASSERT(vkEndCommandBuffer(vulkan_state->command_buffers[current_frame]));

	VkSemaphore wait_semaphores[] = {
		vulkan_state->compute_finished_semaphores[current_frame],
		vulkan_state->image_available_semaphores[current_frame]
	};

	VkSemaphore signal_semaphores[] = {
		vulkan_state->render_finished_semaphores[current_frame]
	};

	VkPipelineStageFlags wait_stages[] = {
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 2,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan_state->command_buffers[current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores
	};

	transition_image_layout(
			vulkan_state,
			vulkan_state->render_texture_image,
			vulkan_state->surface_format.format,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VK_ASSERT(vkQueueSubmit(vulkan_state->graphics_queue, 1, &submit_info, vulkan_state->in_flight_fences[current_frame]));

	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = &vulkan_state->swapchain,
		.pImageIndices = &image_index
	};

	vkQueuePresentKHR(vulkan_state->present_queue, &present_info);
}

__declspec(dllexport)
void game_update_and_render() {
	// Check for changes to compute shader
	uint64 compute_shader_modified_time = platform_get_file_modified_time("data/shaders/compute.spv");
	if (compute_shader_modified_time > GAME_MEMORY->prev_compute_shader_modified_time) {
		GAME_MEMORY->prev_compute_shader_modified_time = compute_shader_modified_time;

		VulkanState* vulkan_state = &GAME_MEMORY->vulkan_state;

		// Create shader module
		uint32 compute_shader_code_size;
		char* compute_shader_code = platform_read_file("data/shaders/compute.spv", &compute_shader_code_size);

		VkShaderModuleCreateInfo compute_shader_create_info = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = compute_shader_code_size,
			.pCode = (uint32*)compute_shader_code
		};

		VkShaderModule compute_shader_module;
		vkCreateShaderModule(vulkan_state->device, &compute_shader_create_info, NULL, &compute_shader_module);

		VkPipelineShaderStageCreateInfo compute_shader_stage_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = compute_shader_module,
			.pName = "main"
		};

		// Create pipeline
		VkComputePipelineCreateInfo compute_pipeline_create_info = {
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.layout = vulkan_state->compute_pipeline_layout,
			.stage = compute_shader_stage_create_info
		};

		VK_ASSERT(vkCreateComputePipelines(
				vulkan_state->device,
				VK_NULL_HANDLE,
				1, &compute_pipeline_create_info,
				NULL,
				&vulkan_state->compute_pipeline));

		printf("Rebuilt compute pipeline\n");
	}

	// Draw
	RENDERER_STATE.time = PLATFORM_DATA->total_time;
	draw_frame(&GAME_MEMORY->vulkan_state, GAME_MEMORY->current_frame, RENDERER_STATE);
	GAME_MEMORY->current_frame = (GAME_MEMORY->current_frame + 1) % 2;
}

__declspec(dllexport)
void game_init(
		PlatformData* platform_data,
		VulkanPlatformData vulkan_platform_data,
		void* memory)
{
	load_vulkan(vulkan_platform_data);

	GAME_MEMORY = memory;
	GAME_MEMORY->vulkan_state = GAME_MEMORY->vulkan_state;

	PLATFORM_DATA = platform_data;

	if (!GAME_MEMORY->vulkan_state.is_initialized) {
		GAME_MEMORY->vulkan_state = setup_renderer(vulkan_platform_data, platform_data->window_width, platform_data->window_height);
		GAME_MEMORY->prev_compute_shader_modified_time = platform_get_file_modified_time("data/shaders/compute.spv");
		GAME_MEMORY->current_frame = 0;
	}

	float focal_length = 1;
	float viewport_height = 2;
	float viewport_width = viewport_height * ((float)320 / 180);

	Float3 viewport_u = float3(viewport_width, 0, 0);
	Float3 viewport_v = float3(0, -viewport_height, 0);
	Float3 pixel_delta_u = float3_div(viewport_u, 320);
	Float3 pixel_delta_v = float3_div(viewport_v, 180);
	Float3 viewport_upper_left = float3_sub(float3(0, 0, -focal_length), float3_add(float3_div(viewport_u, 2), float3_div(viewport_v, 2)));
	Float3 first_pixel_location = float3_add(viewport_upper_left, float3_scale(float3_add(pixel_delta_u, pixel_delta_v), 0.5f));

	RENDERER_STATE = (RendererState){
		.sample_count = 16,
		.pixel_delta_u = pixel_delta_u,
		.pixel_delta_v = pixel_delta_v,
		.first_pixel_location = first_pixel_location
	};

	// Load assets
	Triangle* triangles;
	uint32 num_triangles = parse_obj(platform_read_file("data/assets/suzanne.obj", NULL), &triangles);
	printf("Triangle count: %i\n", num_triangles);

	// Pack and send triangles to GPU
	FOR(i, num_triangles) {
		GPUTriangle gpu_triangle;
		FOR(j, 3) {
			memcpy(gpu_triangle.vertices[j].array, triangles[i].vertices[j].array, 3 * sizeof(float));
		}

		memcpy(
				(GPUTriangle*)GAME_MEMORY->vulkan_state.triangle_buffer_mapped + i,
				&gpu_triangle,
				sizeof(GPUTriangle));
	}
	RENDERER_STATE.num_triangles = num_triangles;

	BVHNodeFlat* bvh_nodes;
	uint32 num_bvh_nodes = build_bvh(triangles, num_triangles, &bvh_nodes);
	memcpy(GAME_MEMORY->vulkan_state.bvh_buffer_mapped, bvh_nodes, num_bvh_nodes * sizeof(BVHNodeFlat));
	RENDERER_STATE.num_bvh_nodes = num_bvh_nodes;

	printf("BVH size: %i nodes\n", num_bvh_nodes);
}
