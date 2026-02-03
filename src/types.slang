struct RendererState {
	float3 pixel_delta_u;
	float3 pixel_delta_v;
	float3 first_pixel_location;
	uint sample_count;
	float time;
	uint num_triangles;
	uint num_bvh_nodes;
}

struct BVHNode {
	float[3] min_bounds;
	float[3] max_bounds;
	uint offset; // Offset for triangles for leaves, offset for right child for interior
	uint32_t num_triangles;
}

struct Sphere {
	float3 center;
	float radius;
	float3 color;
}

struct Tri {
	float3[3] verts;
};

struct Ray {
	float3 start;
	float3 direction;
};

struct HitInfo {
	bool hit;
	float t;
	float3 intersect_point;
	float3 normal;
	float3 color;
};
