struct RendererState {
	float3 pixel_delta_u;
	float3 pixel_delta_v;
	float3 first_pixel_location;
	uint sample_count;
	float time;
	uint num_objects;
	uint num_lights;
	uint num_frames;
	float4x4 camera_transform;
	bool progressive;
}

struct BVHNode {
	float[3] min_bounds;
	float[3] max_bounds;
	uint32_t offset; // Offset for triangles for leaves, offset for right child for interior
	uint32_t num_primitives;
}

struct Sphere {
	float3 center;
	float radius;
	float3 color;
}

struct Vert {
	float3 position;
	float3 normal;
	float2 tex_coord;
}

struct Tri {
	Vert[3] verts;
}

struct Ray {
	float3 start;
	float3 direction;
}

struct HitInfo {
	bool hit;
	float t;
	float3 intersect_point;
	Tri tri;
	float u;
	float v;
}

struct HitResult {
	float t;
	Ray ray;
	uint object_index;
	/*
	Tri tri;
	float u;
	float v;
	*/
	float3 color;
	float3 normal;
}

struct Material {
	float3 color;
	float roughness;
}

struct Object {
	float4x4 transform;

	float4x4 inv_transform;

	uint32_t bvh_root_offset;
	uint32_t triangle_offset;
	bool is_light;

	Material material;
}

struct RayDebugInfo {
	uint tri_tests;
	uint mesh_aabb_tests;
	uint scene_aabb_tests;
	uint scene_nodes;
}

struct Angle {
	float theta;
	float phi;

	Angle operator+(Angle other) {
		return { theta + other.theta, phi + other.phi };
	}
}
