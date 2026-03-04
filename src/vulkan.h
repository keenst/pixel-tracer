enum { MAX_FRAMES_IN_FLIGHT = 2 };
enum { MAX_TRIANGLE_BUFFER_COUNT = 4096 };
enum { MAX_BVH_BUFFER_COUNT = 4096 };
enum { MAX_OBJECT_BUFFER_COUNT = 64 };

typedef struct {
	Mat4 transform;
	Mat4 inv_transform;
	uint32 bvh_root_offset;
	uint32 triangle_offset;
	int32 pad[2];
} RenderObject;

typedef struct {
	bool is_initialized;

	VkDevice device;
	VkPhysicalDevice physical_device;

	VkSurfaceKHR surface;
	VkSurfaceFormatKHR surface_format;

	VkSwapchainKHR swapchain;

	uint32 image_view_count;
	VkFramebuffer* framebuffers;
	VkImageView* image_views;
	VkImage render_texture_image;

	VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
	VkCommandBuffer compute_command_buffers[MAX_FRAMES_IN_FLIGHT];

	VkBuffer renderer_state_buffers[MAX_FRAMES_IN_FLIGHT];
	VkDeviceMemory renderer_state_buffers_memory[MAX_FRAMES_IN_FLIGHT];
	void* renderer_state_buffers_mapped[MAX_FRAMES_IN_FLIGHT];
	VkBuffer object_buffers[MAX_FRAMES_IN_FLIGHT];
	VkDeviceMemory object_buffers_memory[MAX_FRAMES_IN_FLIGHT];
	void* object_buffers_mapped[MAX_FRAMES_IN_FLIGHT];
	VkBuffer triangle_buffer;
	VkDeviceMemory triangle_buffer_memory;
	void* triangle_buffer_mapped;
	VkBuffer bvh_buffer;
	VkDeviceMemory bvh_buffer_memory;
	void* bvh_buffer_mapped;

	VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSet compute_descriptor_sets[MAX_FRAMES_IN_FLIGHT];
	VkCommandPool command_pool;
	VkPipelineLayout graphics_pipeline_layout;
	VkPipelineLayout compute_pipeline_layout;
	VkPipeline graphics_pipeline;
	VkPipeline compute_pipeline;
	VkQueue graphics_queue;
	VkQueue present_queue;

	VkPresentModeKHR present_mode;

	VkExtent2D swap_extent;
	VkRenderPass render_pass;

	uint32 present_family;
	uint32 graphics_family;

	VkViewport viewport;
	VkRect2D scissor;

	VkCommandBufferBeginInfo command_buffer_begin_info;
	VkCommandBufferBeginInfo compute_command_buffer_begin_info;
	VkRenderPassBeginInfo render_pass_begin_info;

	VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore compute_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence compute_in_flight_fences[MAX_FRAMES_IN_FLIGHT];
} VulkanState;

typedef struct {
	Vec3 pixel_delta_u;
	byte pad_0[4];
	Vec3 pixel_delta_v;
	byte pad_1[4];
	Vec3 first_pixel_location;
	uint32 sample_count;
	float time;
	uint32 num_objects;
} RendererState;
