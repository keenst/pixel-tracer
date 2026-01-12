typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t byte;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef union {
	float array[3];
	struct { float x, y, z; };
	struct { float r, g, b; };
} Float3;

typedef union {
	uint32 array[3];
	struct { uint32 x, y, z; };
} Uint3;
