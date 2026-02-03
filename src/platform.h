typedef enum {
	MBOX_ASSERTION
} MessageBoxType;

typedef struct {
	uint32 window_width;
	uint32 window_height;
	float delta_time;
	float total_time;
} PlatformData;

typedef struct {
	VkInstance instance;
	VkSurfaceKHR surface;

	PFN_vkGetInstanceProcAddr func_vkGetInstanceProcAddr;
	PFN_vkEnumerateInstanceLayerProperties func_vkEnumerateInstanceLayerProperties; // NOTE: This could probably be loaded by application layer?

	char** enabled_layers;
	uint32 enabled_layer_count;
	char** device_extensions;
	uint32 device_extension_count;
} VulkanPlatformData;

typedef struct {
	bool f1, f2, f3, f4, f5, f6,
		 f7, f8, f9, f10, f11, f12;
} Inputs;

char* platform_read_file(char* path, uint32* out_size);
int platform_message_box(char* caption, char* text, MessageBoxType type);
void platform_quit();
uint64 platform_get_file_modified_time(char* path);
