typedef enum {
	RM_NORMAL,
	RM_DEBUG,
	RM_COUNT
} RenderMode;

typedef uint32 ObjectID;
typedef uint32 MeshID;

typedef struct {
	Vec3 position;
	Vec3 orientation;
	Vec3 scale;
	MeshID mesh_id;
	Material material;
	int scene_id;
} Object;

typedef struct {
	uint32 root_node_offset;
	uint32 triangle_offset;
	Vec3 centroid;
	Vec3 min_bounds;
	Vec3 max_bounds;
} Mesh;

typedef struct {
	Mesh* mesh;
	Mat4 inv_transform;
} SceneBVHObject;

enum { NAME_LEN = 64 };

typedef struct GlobalMemory_ {
	VulkanState vulkan_state;
	PlatformData* platform_data;
	RendererState renderer_state;

	Arena* arena_stack[ARENA_STACK_MAX];
	int arena_stack_size;
	Arena base_arena;
	Arena frame_arena;
	Arena asset_arena;

	Inputs prev_inputs;

	uint64 prev_compute_shader_modified_time;
	uint32 current_frame;
	RenderMode current_render_mode;

	int current_scene_id;

	Object objects[64];
	char object_names[NAME_LEN][64];
	uint32 num_objects;

	Mesh meshes[64];
	char mesh_names[NAME_LEN][64];
	uint32 num_meshes;

	BVHNodeFlat* bvh_buffer;
	uint bvh_buffer_size;
	Triangle* triangle_buffer;
	uint triangle_buffer_size;
} GlobalMemory;

typedef struct {
	float arr[2];
	float pad[2];
} GPUVec2;

typedef struct {
	float arr[3];
	float pad;
} GPUVec3;

typedef struct {
	GPUVec3 position;
	GPUVec3 normal;
	GPUVec2 tex_coord;
} GPUVertex;

typedef struct {
	GPUVertex vertices[3];
} GPUTriangle;
