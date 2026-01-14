HMODULE GAME_DLL;

uint32 WINDOW_WIDTH = 1600;
uint32 WINDOW_HEIGHT = 900;

typedef void (*GAME_UPDATE_AND_RENDER)(void);
GAME_UPDATE_AND_RENDER game_update_and_render;
typedef void (*GAME_INIT)(PlatformData*, VulkanPlatformData, void*);
GAME_INIT game_init;

double win32_get_time_ms() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	return (counter.QuadPart / (double)frequency.QuadPart) * 1000;
}

bool win32_load_app() {
	FILE* source = CreateFileA("build/pixel_tracer.dll", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (source == INVALID_HANDLE_VALUE) {
		return false;
	}
	CloseHandle(source);

	FreeLibrary(GAME_DLL);

	FILE* destination = CreateFileA("build/pixel_tracer_temp.dll", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (destination == INVALID_HANDLE_VALUE) {
		return false;
	}
	CloseHandle(destination);

	if (!CopyFile("build/pixel_tracer.dll", "build/pixel_tracer_temp.dll", false)) {
		printf("Failed to copy app DLL");
		return false;
	}

	GAME_DLL = LoadLibraryA("build/pixel_tracer_temp.dll");

	if (GAME_DLL) {
		game_update_and_render = (GAME_UPDATE_AND_RENDER)GetProcAddress(GAME_DLL, "game_update_and_render");
		game_init = (GAME_INIT)GetProcAddress(GAME_DLL, "game_init");
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
	uint64 prev_load_time = platform_get_file_modified_time("build/pixel_tracer.dll");

	VulkanPlatformData vulkan_platform_data = win32_init_vulkan(window, instance);

	PlatformData platform_data = {
		.window_width = WINDOW_WIDTH,
		.window_height = WINDOW_HEIGHT,
		.delta_time = 0,
		.total_time = 0
	};

	void* game_memory = VirtualAlloc(NULL, 1024 * 1024 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	game_init(&platform_data, vulkan_platform_data, game_memory);

	double last_update_time = win32_get_time_ms();
	double frame_time;

	const double perf_update_interval = 1000;
	double perf_update_timer = 0;
	uint64 frame_count = 0;

	MSG message;
	bool running = true;
	while (running) {
		double time_now = win32_get_time_ms();
		frame_time = time_now - last_update_time;
		last_update_time = time_now;

		platform_data.delta_time = frame_time / 1000;
		platform_data.total_time += frame_time / 1000;

		perf_update_timer += frame_time;
		if (perf_update_timer >= perf_update_interval) {
			char title_buffer[128];
			snprintf(title_buffer, 128, "FPS: %llu (%.3f ms)", frame_count, perf_update_timer / frame_count);
			SetWindowTextA(window, title_buffer);

			perf_update_timer = 0;
			frame_count = 0;
		}

		uint64 load_time = platform_get_file_modified_time("build/pixel_tracer.dll");
		if (load_time != prev_load_time) {
			// Check if file is in use by another process
			FILE* file = CreateFileA("build/pixel_tracer.dll", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file) {
				CloseHandle(file);
				if (win32_load_app()) {
					prev_load_time = load_time;

					system("cls");
					game_init(&platform_data, vulkan_platform_data, game_memory);
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

		game_update_and_render();

		frame_count++;
	}
}
