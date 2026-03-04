typedef struct {
	Vec3 position;
	Vec3 normal;
	Vec2 tex_coord;
} Vertex;

typedef struct {
	Int3 v;
	Int3 vt;
	Int3 vn;
} Face;

typedef struct {
	Vertex vertices[3];
	Vec3 centroid; // Used when building BVH
} Triangle;
