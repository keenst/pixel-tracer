struct RendererState {
	float3 pixel_delta_u;
	float3 pixel_delta_v;
	float3 first_pixel_location;
	uint sample_count;
	float time;
	uint num_objects;
}

struct BVHNode {
	float[3] min_bounds;
	float[3] max_bounds;
	uint32_t offset; // Offset for triangles for leaves, offset for right child for interior
	uint32_t num_triangles;
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
};

struct Tri {
	Vert[3] verts;
};

struct Ray {
	float3 start;
	float3 direction;
};

struct HitInfo {
	bool hit;
	float t;
	float3 color;
	float3 intersect_point;
	float3 normal;
};

struct HitResult {
	float t;
	Ray ray;
	Object object;
}

struct Object {
	float4x4 transform;
	float4x4 inv_transform;
	uint32_t bvh_root_offset;
	uint32_t triangle_offset;
};

struct RayDebugInfo {
	uint tri_tests;
	uint aabb_tests;
};
