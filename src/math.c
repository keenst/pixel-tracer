Vec3f vec3f(f32 x, f32 y, f32 z) {
	return (Vec3f){ .x = x, .y = y, .z = z };
}

Vec3f vec3f_div(Vec3f a, f32 b) {
	return vec3f(a.x / b, a.y / b, a.z / b);
}

Vec3f vec3f_scale(Vec3f a, f32 b) {
	return vec3f(a.x * b, a.y * b, a.z * b);
}

Vec3f vec3f_add(Vec3f a, Vec3f b) {
	return vec3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3f vec3f_sub(Vec3f a, Vec3f b) {
	return vec3f(a.x - b.x, a.y - b.y, a.z - b.z);
}
