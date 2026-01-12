#include <windows.h>
#include <stdint.h>
#include <stdio.h>
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
