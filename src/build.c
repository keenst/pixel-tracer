#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

#define FOR(i, n) for(uint32 i = 0; i < n; i++)
#define FOR_RANGE(i, start, end) for (uint32 i = start; i <= end; i++)

struct GlobalMemory_;
struct GlobalMemory_* global;

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"
#include "vulkan_functions.h"

#include "types.h"
#include "assert.c"
#include "platform.h"

// Headers
#include "memory.h"
#include "math.h"
#include "parse_obj.h"
#include "bvh.h"
#include "vulkan.h"
#include "main.h"

// Source
#include "memory.c"
#include "math.c"
#include "parse_obj.c"
#include "bvh.c"
#include "vulkan.c"
#include "main.c"
