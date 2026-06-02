HMODULE GAME_DLL;

uint32 WINDOW_WIDTH = 1920;
uint32 WINDOW_HEIGHT = 1080;

typedef void (*GAME_UPDATE_AND_RENDER)(Inputs inputs);
GAME_UPDATE_AND_RENDER game_update_and_render;
typedef void (*GAME_INIT)(PlatformData*, VulkanPlatformData, void*, uint);
GAME_INIT game_init;

char* GAME_PATH = "../build/pixel_tracer.dll";
char* TEMP_GAME_PATH = "../build/pixel_tracer_temp.dll";

Inputs INPUTS = {};

bool global_mouse_locked = false;

double win32_get_time_ms() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	return (counter.QuadPart / (double)frequency.QuadPart) * 1000;
}

bool win32_load_app() {
	FILE* source = CreateFileA(GAME_PATH, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (source == INVALID_HANDLE_VALUE) {
		return false;
	}
	CloseHandle(source);

	FreeLibrary(GAME_DLL);

	FILE* destination = CreateFileA(TEMP_GAME_PATH, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (destination == INVALID_HANDLE_VALUE) {
		return false;
	}
	CloseHandle(destination);

	if (!CopyFile(GAME_PATH, TEMP_GAME_PATH, false)) {
		printf("Failed to copy app DLL");
		return false;
	}

	GAME_DLL = LoadLibraryA(TEMP_GAME_PATH);

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
#define KEY_DOWN(win32_key, key) \
	case win32_key: INPUTS.key = true; break;
#define KEY_UP(win32_key, key) \
	case win32_key: INPUTS.key = false; break;

	switch (message) {
		case WM_DESTROY: {
			PostQuitMessage(0);
		} return 0;
		case WM_KEYDOWN: {
			switch (w_param) {
				KEY_DOWN(VK_F1, f1)
				KEY_DOWN(VK_F2, f2)
				KEY_DOWN(VK_F3, f3)
				KEY_DOWN(VK_F4, f4)
				KEY_DOWN(VK_F5, f5)
				KEY_DOWN(VK_F6, f6)
				KEY_DOWN(VK_F7, f7)
				KEY_DOWN(VK_F8, f8)
				KEY_DOWN(VK_F9, f9)
				KEY_DOWN(VK_F10, f10)
				KEY_DOWN(VK_F11, f11)
				KEY_DOWN(VK_F12, f12)
				KEY_DOWN('P', p)
				KEY_DOWN('W', w)
				KEY_DOWN('A', a)
				KEY_DOWN('S', s)
				KEY_DOWN('D', d)
				KEY_DOWN(VK_CONTROL, ctrl)
				KEY_DOWN(VK_SPACE, space)
				KEY_DOWN(VK_SHIFT, shift)
				default: {
					if (w_param >= '0' && w_param <= '9') {
						INPUTS.nums[w_param - '0'] = true;
					}
				} break;
			}
		} break;
		case WM_KEYUP: {
			switch (w_param) {
				KEY_UP(VK_F1, f1)
				KEY_UP(VK_F2, f2)
				KEY_UP(VK_F3, f3)
				KEY_UP(VK_F4, f4)
				KEY_UP(VK_F5, f5)
				KEY_UP(VK_F6, f6)
				KEY_UP(VK_F7, f7)
				KEY_UP(VK_F8, f8)
				KEY_UP(VK_F9, f9)
				KEY_UP(VK_F10, f10)
				KEY_UP(VK_F11, f11)
				KEY_UP(VK_F12, f12)
				KEY_UP('P', p)
				KEY_UP('W', w)
				KEY_UP('A', a)
				KEY_UP('S', s)
				KEY_UP('D', d)
				KEY_UP(VK_CONTROL, ctrl)
				KEY_UP(VK_SPACE, space)
				KEY_UP(VK_SHIFT, shift)
				default: {
					if (w_param >= '0' && w_param <= '9') {
						INPUTS.nums[w_param - '0'] = false;
					}
				} break;
			}
		} break;
		case WM_MOUSEMOVE: {
			POINT cursor_pos = {};
			GetCursorPos(&cursor_pos);

			float mouse_x = (float)cursor_pos.x / WINDOW_WIDTH;
			float mouse_y = (float)cursor_pos.y / WINDOW_HEIGHT;

			INPUTS.mouse_delta_x = INPUTS.mouse_x - mouse_x;
			INPUTS.mouse_delta_y = INPUTS.mouse_y - mouse_y;

			if (global_mouse_locked) {
				RECT window_rect;
				GetWindowRect(GetActiveWindow(), &window_rect);

				int x = window_rect.left;
				int y = window_rect.top;
				int width = window_rect.right - window_rect.left;
				int height = window_rect.bottom - window_rect.top;

				SetCursorPos(width / 2 + x, height / 2 + y);

				GetCursorPos(&cursor_pos);

				mouse_x = (float)cursor_pos.x / WINDOW_WIDTH;
				mouse_y = (float)cursor_pos.y / WINDOW_HEIGHT;
			}

			INPUTS.mouse_x = mouse_x;
			INPUTS.mouse_y = mouse_y;
		} break;
		case WM_LBUTTONDOWN: {
			INPUTS.left_mouse = true;
		} break;
		case WM_LBUTTONUP: {
			INPUTS.left_mouse = false;
		} break;
		case WM_RBUTTONDOWN: {
			INPUTS.right_mouse = true;
		} break;
		case WM_RBUTTONUP: {
			INPUTS.right_mouse = false;
		} break;
	}

#undef KEY_DOWN
#undef KEY_UP

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
	uint64 prev_load_time = platform_get_file_modified_time(GAME_PATH);

	VulkanPlatformData vulkan_platform_data = win32_init_vulkan(window, instance);

	PlatformData platform_data = {
		.window_width = WINDOW_WIDTH,
		.window_height = WINDOW_HEIGHT,
		.delta_time = 0
	};

	uint game_memory_size = 1024 * 1024 * 1024;
	void* game_memory = VirtualAlloc(NULL, game_memory_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	game_init(&platform_data, vulkan_platform_data, game_memory, game_memory_size);

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

		perf_update_timer += frame_time;
		if (perf_update_timer >= perf_update_interval) {
			char title_buffer[128];
			snprintf(title_buffer, 128, "FPS: %llu (%.3f ms)", frame_count, perf_update_timer / frame_count);
			SetWindowTextA(window, title_buffer);

			perf_update_timer = 0;
			frame_count = 0;
		}

		uint64 load_time = platform_get_file_modified_time(GAME_PATH);
		if (load_time != prev_load_time) {
			// Check if file is in use by another process
			FILE* file = CreateFileA(GAME_PATH, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file) {
				CloseHandle(file);
				if (win32_load_app()) {
					prev_load_time = load_time;

					system("cls");
					game_init(&platform_data, vulkan_platform_data, game_memory, game_memory_size);
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

		game_update_and_render(INPUTS);

		if (platform_data.mouse_locked != global_mouse_locked) {
			global_mouse_locked = platform_data.mouse_locked;
			ShowCursor(!platform_data.mouse_locked);
		}

		frame_count++;
	}
}
