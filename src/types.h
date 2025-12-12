typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t byte;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef union {
	f32 array[3];
	struct { f32 x, y, z; };
	struct { f32 r, g, b; };
} Vec3f;
