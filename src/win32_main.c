#include "win32_unity.h"

HMODULE APP_DLL;

u32 WINDOW_WIDTH = 1600;
u32 WINDOW_HEIGHT = 900;

VulkanState VULKAN_STATE = {};
u32 CURRENT_FRAME = 0;

typedef void (*UPDATE_AND_RENDER)(void);
UPDATE_AND_RENDER update_and_render;

f64 win32_get_time_ms() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	return (counter.QuadPart / (f64)frequency.QuadPart) * 1000;
}

int win32_message_box(char* caption, char* text, u32 type) {
	HWND window = GetActiveWindow();
	return MessageBoxA(window, text, caption, type);
}

bool win32_load_app() {
	FILE* source = CreateFileA("build/pixel_tracer.dll", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (source == INVALID_HANDLE_VALUE) {
		return false;
	}
	CloseHandle(source);

	FreeLibrary(APP_DLL);

	FILE* destination = CreateFileA("build/pixel_tracer_temp.dll", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (destination == INVALID_HANDLE_VALUE) {
		return false;
	}
	CloseHandle(destination);

	if (!CopyFile("build/pixel_tracer.dll", "build/pixel_tracer_temp.dll", false)) {
		printf("Failed to copy app DLL");
		return false;
	}

	APP_DLL = LoadLibraryA("build/pixel_tracer_temp.dll");

	if (APP_DLL) {
		update_and_render = (UPDATE_AND_RENDER)GetProcAddress(APP_DLL, "update_and_render");
	} else {
		printf("Failed to load app DLL");
		return false;
	}

	return true;
}

void win32_draw_frame(VulkanState* vulkan_state, u32 current_frame) {
	// Compute
	VK_ASSERT(vkWaitForFences(vulkan_state->device, 1, &vulkan_state->compute_in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));
	VK_ASSERT(vkResetFences(vulkan_state->device, 1, &vulkan_state->compute_in_flight_fences[current_frame]));

	VK_ASSERT(vkResetCommandBuffer(vulkan_state->compute_command_buffers[current_frame], 0));
	VK_ASSERT(vkBeginCommandBuffer(vulkan_state->compute_command_buffers[current_frame], &vulkan_state->command_buffer_begin_info));

	vkCmdBindPipeline(vulkan_state->compute_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_COMPUTE, vulkan_state->compute_pipeline);
	vkCmdBindDescriptorSets(vulkan_state->compute_command_buffers[current_frame], VK_PIPELINE_BIND_POINT_COMPUTE, vulkan_state->compute_pipeline_layout, 0, 1, &vulkan_state->compute_descriptor_sets[current_frame], 0, NULL);
	vkCmdDispatch(vulkan_state->compute_command_buffers[current_frame], 320, 180, 1);

	VK_ASSERT(vkEndCommandBuffer(vulkan_state->compute_command_buffers[current_frame]));

	VkSemaphore compute_signal_semaphores[] = { vulkan_state->compute_finished_semaphores[current_frame] };
	VkSubmitInfo compute_submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan_state->compute_command_buffers[current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = compute_signal_semaphores
	};

	transition_image_layout(vulkan_state, vulkan_state->texture_image, vulkan_state->surface_format.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	VK_ASSERT(vkQueueSubmit(vulkan_state->graphics_queue, 1, &compute_submit_info, vulkan_state->compute_in_flight_fences[current_frame]));

	// Graphics
	VK_ASSERT(vkWaitForFences(vulkan_state->device, 1, &vulkan_state->in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));
	VK_ASSERT(vkResetFences(vulkan_state->device, 1, &vulkan_state->in_flight_fences[current_frame]));

	u32 image_index;
	vkAcquireNextImageKHR(vulkan_state->device, vulkan_state->swapchain, UINT64_MAX, vulkan_state->image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);
	vulkan_state->render_pass_begin_info.framebuffer = vulkan_state->framebuffers[image_index];

	VK_ASSERT(vkResetCommandBuffer(vulkan_state->command_buffers[current_frame], 0));
	VK_ASSERT(vkBeginCommandBuffer(vulkan_state->command_buffers[current_frame], &vulkan_state->command_buffer_begin_info));

	vkCmdBeginRenderPass(vulkan_state->command_buffers[current_frame], &vulkan_state->render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(vulkan_state->command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_state->graphics_pipeline);

	vkCmdSetViewport(vulkan_state->command_buffers[current_frame], 0, 1, &vulkan_state->viewport);
	vkCmdSetScissor(vulkan_state->command_buffers[current_frame], 0, 1, &vulkan_state->scissor);

	vkCmdBindDescriptorSets(vulkan_state->command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_state->graphics_pipeline_layout, 0, 1, &vulkan_state->descriptor_sets[current_frame], 0, NULL);

	vkCmdDraw(vulkan_state->command_buffers[current_frame], 6, 1, 0, 0);

	vkCmdEndRenderPass(vulkan_state->command_buffers[current_frame]);

	VK_ASSERT(vkEndCommandBuffer(vulkan_state->command_buffers[current_frame]));

	Vec3f uniform_color = {
		.r = 1.0f,
		.g = 0.0f,
		.b = 0.0f
	};

	memcpy(vulkan_state->uniform_buffers_mapped[current_frame], &uniform_color, sizeof(uniform_color));

	VkSemaphore wait_semaphores[] = { vulkan_state->compute_finished_semaphores[current_frame], vulkan_state->image_available_semaphores[current_frame] };
	VkSemaphore signal_semaphores[] = { vulkan_state->render_finished_semaphores[current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

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

	transition_image_layout(vulkan_state, vulkan_state->texture_image, vulkan_state->surface_format.format, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
	switch (message) {
		case WM_DESTROY: {
			PostQuitMessage(0);
		} return 0;
		case WM_SIZE: {
			if (!VULKAN_STATE.is_initialized) {
				break;
			}

			RECT client_rect = {
				.left = 0,
				.top = 0,
				.right = LOWORD(l_param),
				.bottom = HIWORD(l_param)
			};
			AdjustWindowRect(&client_rect, WS_OVERLAPPEDWINDOW, 0);

			WINDOW_WIDTH = client_rect.right - client_rect.left;
			WINDOW_HEIGHT = client_rect.bottom - client_rect.top;

			win32_create_swapchain(&VULKAN_STATE, WINDOW_WIDTH, WINDOW_HEIGHT);
			VULKAN_STATE.viewport.width = VULKAN_STATE.swap_extent.width;
			VULKAN_STATE.viewport.height = VULKAN_STATE.swap_extent.height;
			VULKAN_STATE.scissor.extent = VULKAN_STATE.swap_extent;
			VULKAN_STATE.render_pass_begin_info.renderArea.extent = VULKAN_STATE.swap_extent;

			win32_draw_frame(&VULKAN_STATE, CURRENT_FRAME);
		} break;
	}

	return DefWindowProc(window, message, w_param, l_param);
}

FILETIME win32_get_modified_time(char* path) {
	WIN32_FIND_DATA file_data = {};
	FILE* file = FindFirstFile(path, &file_data);
	assert(file);
	FindClose(file);
	return file_data.ftLastWriteTime;
}

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show) {
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	const char* class_name = "Pixel tracer";
	WNDCLASS window_class = {
		.lpfnWndProc = WindowProc,
		.hInstance = instance,
		.lpszClassName = class_name
	};
	RegisterClass(&window_class);

	RECT client_rect = {
		.left = 0,
		.top = 0,
		.right = WINDOW_WIDTH,
		.bottom = WINDOW_HEIGHT
	};
	AdjustWindowRect(&client_rect, WS_OVERLAPPEDWINDOW, 0);

	HWND window = CreateWindowEx(
		0,
		class_name,
		class_name,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		client_rect.right - client_rect.left,
		client_rect.bottom - client_rect.top,
		NULL, NULL,
		instance, NULL);

	assert(window);

	ShowWindow(window, cmd_show);

	win32_load_app();
	FILETIME prev_load_time = win32_get_modified_time("build/pixel_tracer.dll");

	VULKAN_STATE = win32_init_vulkan(window, instance, WINDOW_WIDTH, WINDOW_HEIGHT);

	f64 last_update_time = win32_get_time_ms();
	f64 frame_time;

	const f64 perf_update_interval = 1000;
	f64 perf_update_timer = 0;
	u64 frame_count = 0;

	MSG message;
	bool running = true;
	while (running) {
		f64 time_now = win32_get_time_ms();
		frame_time = time_now - last_update_time;
		last_update_time = time_now;

		CURRENT_FRAME = (CURRENT_FRAME + 1) % MAX_FRAMES_IN_FLIGHT;

		perf_update_timer += frame_time;
		if (perf_update_timer >= perf_update_interval) {
			char title_buffer[128];
			snprintf(title_buffer, 128, "FPS: %llu (%f ms)", frame_count, frame_time);
			SetWindowTextA(window, title_buffer);

			perf_update_timer = 0;
			frame_count = 0;
		}

		FILETIME load_time = win32_get_modified_time("build/pixel_tracer.dll");
		if (CompareFileTime(&load_time, &prev_load_time)) {
			// Check if file is in use by another process
			FILE* file = CreateFileA("build/pixel_tracer.dll", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file) {
				CloseHandle(file);
				if (win32_load_app()) {
					prev_load_time = load_time;
				}
			}
		}

		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT) {
				running = false;
				break;
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		win32_draw_frame(&VULKAN_STATE, CURRENT_FRAME);

		update_and_render();

		frame_count++;
	}
}
