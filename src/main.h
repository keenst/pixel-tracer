typedef struct {
	VulkanState vulkan_state;
	uint64 prev_compute_shader_modified_time;
	uint32 current_frame;
} GameMemory;
