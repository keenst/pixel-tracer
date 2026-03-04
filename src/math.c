Vec3 vec3(float x, float y, float z) {
	return (Vec3){ .x = x, .y = y, .z = z };
}

Vec3 vec3_div(Vec3 a, float b) {
	return vec3(a.x / b, a.y / b, a.z / b);
}

Vec3 vec3_scale(Vec3 a, float b) {
	return vec3(a.x * b, a.y * b, a.z * b);
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
	return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
	return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

float vec3_dot(Vec3 a, Vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float vec4_dot(Vec4 a, Vec4 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

uint32 uint32_clamp(uint32 min, uint32 value, uint32 max) {
	if (value > max) {
		return max;
	}

	if (value < min) {
		return min;
	}

	return value;
}

Mat4 mat4_rot_x(float angle) {
	return (Mat4){
		1, 0, 0, 0,
		0, cosf(angle), -sinf(angle), 0,
		0, sinf(angle), cosf(angle), 0,
		0, 0, 0, 1
	};
}

Mat4 mat4_rot_y(float angle) {
	return (Mat4){
		cosf(angle), 0, sinf(angle), 0,
		0, 1, 0, 0,
		-sinf(angle), 0, cosf(angle), 0,
		0, 0, 0, 1
	};
}

Mat4 mat4_rot_z(float angle) {
	return (Mat4){
		cosf(angle), sinf(angle), 0, 0,
		-sinf(angle), cosf(angle), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
}

void float_swap(float* a, float* b) {
	float temp = *a;
	*a = *b;
	*b = temp;
}

Mat4 mat4_transpose(Mat4 a) {
	float_swap(&a.arr[0][1], &a.arr[1][0]);
	float_swap(&a.arr[0][2], &a.arr[2][0]);
	float_swap(&a.arr[0][3], &a.arr[3][0]);

	float_swap(&a.arr[1][2], &a.arr[2][1]);
	float_swap(&a.arr[1][3], &a.arr[3][1]);

	float_swap(&a.arr[2][3], &a.arr[3][2]);
	return a;
}

Mat4 mat4_mul(Mat4 a, Mat4 b) {
	b = mat4_transpose(b);
	return (Mat4){
		vec4_dot(a.rows[0], b.rows[0]),
		vec4_dot(a.rows[0], b.rows[1]),
		vec4_dot(a.rows[0], b.rows[2]),
		vec4_dot(a.rows[0], b.rows[3]),

		vec4_dot(a.rows[1], b.rows[0]),
		vec4_dot(a.rows[1], b.rows[1]),
		vec4_dot(a.rows[1], b.rows[2]),
		vec4_dot(a.rows[1], b.rows[3]),

		vec4_dot(a.rows[2], b.rows[0]),
		vec4_dot(a.rows[2], b.rows[1]),
		vec4_dot(a.rows[2], b.rows[2]),
		vec4_dot(a.rows[2], b.rows[3]),

		vec4_dot(a.rows[3], b.rows[0]),
		vec4_dot(a.rows[3], b.rows[1]),
		vec4_dot(a.rows[3], b.rows[2]),
		vec4_dot(a.rows[3], b.rows[3])
	};
}
