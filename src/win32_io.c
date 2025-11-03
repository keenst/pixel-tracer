#include "win32_unity.h"

char* win32_read_file(char* path, u32* size) {
	FILE* file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("Failed to open file: %s\n", path);
		return NULL;
	}

	u32 file_size = GetFileSize(file, NULL);

	char* buffer = malloc(file_size);
	ReadFile(file, buffer, file_size, NULL, NULL);

	if (size) {
		*size = file_size;
	}

	return buffer;
}
