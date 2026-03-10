const float pi = 3.1415926535897932384626;
const float tau = pi * 2;

typedef union {
	float arr[2];
	struct { float x, y; };
	struct { float u, v; };
} Vec2;

typedef union {
	float arr[3];
	struct { float x, y, z; };
	struct { float r, g, b; };
	struct { float pitch, yaw, roll; };
} Vec3;

typedef union {
	float arr[4];
	struct { float x, y, z, w; };
	struct { float r, g, b, a; };
} Vec4;

typedef union {
	uint32 arr[3];
	struct { uint32 x, y, z; };
} Int3;

typedef union {
	float arr[4][4];
	Vec4 rows[4];
} Mat4;
