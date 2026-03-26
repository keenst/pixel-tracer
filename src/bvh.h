typedef struct BVHNode_ {
	struct BVHNode_* left;
	struct BVHNode_* right;
	Vec3 min_bounds;
	Vec3 max_bounds;
	bool is_leaf;

	uint32 first_primitive;
	uint32 num_primitives;
} BVHNode;

typedef struct {
	Vec3 min_bounds;
	Vec3 max_bounds;
	union {
		uint32 primitives_offset; // For leaves
		uint32 right_child_offset;
	};
	uint32 num_primitives;
} BVHNodeFlat;
