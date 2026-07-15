enum { MAX_FRAMES_IN_FLIGHT = 2 };
enum { MAX_TRIANGLE_BUFFER_COUNT = 4096 };
enum { MAX_MESH_BVH_BUFFER_COUNT = 4096 };
enum { MAX_OBJECT_BUFFER_COUNT = 64 };
enum { MAX_SCENE_BVH_BUFFER_COUNT = 128 };

typedef struct {
	Vec3 color;
	float roughness;
} Material;

typedef struct {
	Mat4 transform;

	Mat4 inv_transform;

	uint32 bvh_root_offset;
	uint32 triangle_offset;
	uint32 num_triangles;
	bool32 is_light;

	Material material;
} RenderObject;

typedef struct {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* mapped;
} VulkanBuffer;

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

	VulkanBuffer renderer_state_buffers[MAX_FRAMES_IN_FLIGHT];
	VulkanBuffer object_buffers[MAX_FRAMES_IN_FLIGHT];
	VulkanBuffer object_index_buffers[MAX_FRAMES_IN_FLIGHT];
	VulkanBuffer scene_bvh_buffers[MAX_FRAMES_IN_FLIGHT];
	VulkanBuffer triangle_buffer;
	VulkanBuffer mesh_bvh_buffer;

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
	uint32 num_lights;
	uint32 num_frames;

	Mat4 camera_transform;

	bool32 progressive;
} RendererState;
