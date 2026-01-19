typedef struct {
	VulkanState vulkan_state;
	uint64 prev_compute_shader_modified_time;
	uint32 current_frame;
} GameMemory;

typedef struct {
	float array[3];
	byte pad[4];
} GPUFloat3;

typedef struct {
	GPUFloat3 vertices[3];
} GPUTriangle;
