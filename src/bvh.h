typedef struct BVHNode_ {
	struct BVHNode_* left;
	struct BVHNode_* right;
	Float3 min_bounds;
	Float3 max_bounds;
	bool is_leaf;

	uint32 first_tri;
	uint32 num_triangles;
} BVHNode;

typedef struct {
	Float3 min_bounds;
	Float3 max_bounds;
	union {
		uint32 triangles_offset; // For leaves
		uint32 right_child_offset;
	};
	uint32 num_triangles;
} BVHNodeFlat;
