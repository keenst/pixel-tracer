#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
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

	// TODO: Fix this memory leak somehow?
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

char** platform_read_dir(char* path) {
	WIN32_FIND_DATA find_data;
	HANDLE handle;

	// Get file count
	uint32 file_count = 0;
	handle = FindFirstFile(path, &find_data);
	do {
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			file_count++;
		}
	} while (FindNextFile(handle, &find_data));

	char** files = malloc((file_count + 1) * sizeof(char*));

	// Get file names
	uint32 file_index = 0;
	handle = FindFirstFile(path, &find_data);
	do {
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			const char* file_name = find_data.cFileName;
			uint32 file_name_len = strlen(file_name) + 1;

			files[file_index] = malloc(file_name_len);
			memcpy(files[file_index], file_name, file_name_len + 1);
			file_index++;
		}
	} while (FindNextFile(handle, &find_data));

	FindClose(handle);

	files[file_count] = NULL;
	return files;
}

float platform_get_time_ms()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (count.QuadPart / (float)frequency.QuadPart) * 1000.0f;
}
