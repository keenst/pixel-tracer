#include "win32_unity.h"

const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;

HMODULE APP_DLL;

typedef void (*UPDATE_AND_RENDER)(void);
UPDATE_AND_RENDER update_and_render;

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

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
	switch (message) {
		case WM_DESTROY: {
			PostQuitMessage(0);
		} return 0;
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

	VulkanState vulkan_state = win32_init_vulkan(window, instance, WINDOW_WIDTH, WINDOW_HEIGHT);

	MSG message;
	bool running = true;
	while (running) {
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

		vkWaitForFences(vulkan_state.device, 1, &vulkan_state.in_flight_fence, VK_TRUE, UINT64_MAX);
		vkResetFences(vulkan_state.device, 1, &vulkan_state.in_flight_fence);

		u32 image_index;
		vkAcquireNextImageKHR(vulkan_state.device, vulkan_state.swapchain, UINT64_MAX, vulkan_state.image_available_semaphore, VK_NULL_HANDLE, &image_index);
		vulkan_state.render_pass_begin_info.framebuffer = vulkan_state.framebuffers[image_index];

		vkResetCommandBuffer(vulkan_state.command_buffer, 0);

		if (vkBeginCommandBuffer(vulkan_state.command_buffer, &vulkan_state.command_buffer_begin_info) != VK_SUCCESS) {
			printf("Unable to begin command buffer\n");
		}

		vkCmdBeginRenderPass(vulkan_state.command_buffer, &vulkan_state.render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(vulkan_state.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_state.graphics_pipeline);

		vkCmdSetViewport(vulkan_state.command_buffer, 0, 1, &vulkan_state.viewport);
		vkCmdSetScissor(vulkan_state.command_buffer, 0, 1, &vulkan_state.scissor);

		vkCmdDraw(vulkan_state.command_buffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(vulkan_state.command_buffer);

		if (vkEndCommandBuffer(vulkan_state.command_buffer) != VK_SUCCESS) {
			printf("Unable to end command buffer\n");
		}

		VkSemaphore wait_semaphores[] = { vulkan_state.image_available_semaphore };
		VkSemaphore signal_semaphores[] = { vulkan_state.render_finished_semaphore };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = wait_semaphores,
			.pWaitDstStageMask = wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &vulkan_state.command_buffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signal_semaphores
		};

		if (vkQueueSubmit(vulkan_state.graphics_queue, 1, &submit_info, vulkan_state.in_flight_fence) != VK_SUCCESS) {
			printf("Failed to submit draw command buffer\n");
		}

		VkPresentInfoKHR present_info = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signal_semaphores,
			.swapchainCount = 1,
			.pSwapchains = &vulkan_state.swapchain,
			.pImageIndices = &image_index
		};

		vkQueuePresentKHR(vulkan_state.present_queue, &present_info);

		update_and_render();
	}
}
