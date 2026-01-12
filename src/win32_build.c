#include <windows.h>
#include <fileapi.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <float.h>

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#include "types.h"

#include "platform.h"
#include "assert.c"

#define FOR(i, n) for(uint32 i = 0; i < n; i++)

#include "win32_vulkan.c"
#include "win32_main.c"
