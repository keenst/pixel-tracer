char* stringify_vulkan_result(VkResult res) {
	switch (res) {
		case VK_SUCCESS:
			return "VK_SUCCESS";
		case VK_NOT_READY:
			 return "VK_NOT_READY";
		case VK_TIMEOUT:
			return "VK_TIMEOUT";
		case VK_EVENT_SET:
			return "VK_EVENT_SET";
		case VK_EVENT_RESET:
			return "VK_EVENT_RESET";
		case VK_INCOMPLETE:
			return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:
			return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN:
			return "VK_ERROR_UNKNOWN";
		case VK_ERROR_VALIDATION_FAILED:
			return "VK_ERROR_VALIDATION_FAILED";
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
			return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_FRAGMENTATION:
			return "VK_ERROR_FRAGMENTATION";
		case VK_PIPELINE_COMPILE_REQUIRED:
			return "VK_PIPELINE_COMPILE_REQUIRED";
		case VK_ERROR_NOT_PERMITTED:
			return "VK_ERROR_NOT_PERMITTED";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:
			return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_INVALID_SHADER_NV:
			return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
			return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
			return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR:
			return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR:
			return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR:
			return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR:
			return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
			return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
		case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
			return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
		case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
			return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
		case VK_PIPELINE_BINARY_MISSING_KHR:
			return "VK_PIPELINE_BINARY_MISSING_KHR";
		case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
			return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
		case VK_RESULT_MAX_ENUM:
			return "VK_RESULT_MAX_ENUM";
	}
}

// TODO: Macro is not portable:
// `__builtin_debugtrap()` is clang-specific.
// The IDs for the buttons are windows-specific (create a layer for this, just like with the message box type).
#ifndef NDEBUG
#define _ASSERT(title, message) \
	uint32 button = platform_message_box(title, message, MBOX_ASSERTION); \
	if (button == 3) { /* IDABORT */ \
		platform_quit(); \
	} else if (button == 4) /* IDRETRY */ { \
		__builtin_debugtrap(); \
	}
#else
#define _ASSERT()
#endif

#ifndef NDEBUG
#define ASSERT(expression) \
	if (!(expression)) \
	{ \
		char _message_[256]; \
		sprintf(_message_, \
				"Assertion failed:\n" \
				"Expression: \"" #expression "\"\n" \
				"File: " __FILE__ ":%i\n\n", \
				__LINE__); \
		_ASSERT("Assertion", _message_); \
	}
#else
#define ASSERT()
#endif

#ifndef NDEBUG
#define TRAP(message, ...) \
	{ \
		char _message_[256]; \
		sprintf(_message_, message, __VA_ARGS__); \
		char _trap_message_[256]; \
		sprintf(_trap_message_, \
				"Program hit a trap:\n" \
				"File: " __FILE__ ":%i\n\n" \
				"%s\n", \
				__LINE__, _message_); \
		_ASSERT("Trap", _trap_message_); \
	}
#else
#define TRAP()
#endif

#ifndef NDEBUG
#define VK_ASSERT(function) \
	{ \
		VkResult res = function; \
		if (res != VK_SUCCESS) { \
			char text[256]; \
			snprintf(text, 256, \
					"Vulkan assertion:\n" \
					"File: " __FILE__ ":%i\n" \
					"Result: %s\n\n" \
					#function, \
					__LINE__, stringify_vulkan_result(res)); \
			_ASSERT("Vulkan Assertion", text); \
		} \
	}
#else
#define VK_ASSERT(function) function;
#endif
