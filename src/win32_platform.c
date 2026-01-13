#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "vulkan/vulkan.h"
#include "types.h"

#include "platform.h"

char* platform_read_file(char* path, uint32* out_size) {
	FILE* file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("Failed to open file: %s\n", path);
		return NULL;
	}

	uint32 file_size = GetFileSize(file, NULL);

	char* buffer = malloc(file_size);
	ReadFile(file, buffer, file_size, NULL, NULL);

	if (out_size) {
		*out_size = file_size;
	}

	CloseHandle(file);

	return buffer;
}

int platform_message_box(char* caption, char* text, MessageBoxType type) {
	HWND window = GetActiveWindow();

	uint32 windows_type;
	switch (type) {
		case MBOX_ASSERTION: {
			windows_type = MB_ICONERROR | MB_ABORTRETRYIGNORE;
		} break;
	}

	return MessageBoxA(window, text, caption, windows_type);
}

void platform_quit() {
	PostQuitMessage(0);
}

uint64 platform_get_file_modified_time(char* path) {
	WIN32_FIND_DATA file_data = {};
	FILE* file = FindFirstFile(path, &file_data);
	assert(file);
	FindClose(file);

	FILETIME write_time = file_data.ftLastWriteTime;
	ULARGE_INTEGER integer_time = {
		.LowPart = write_time.dwLowDateTime,
		.HighPart = write_time.dwHighDateTime
	};

	return integer_time.QuadPart;
}
