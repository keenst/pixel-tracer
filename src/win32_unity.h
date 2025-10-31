#pragma once

#include <windows.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#define FOR(i, n) for(uint32_t i = 0; i < n; i++)

#include "types.h"
#include "win32_vulkan.c"
