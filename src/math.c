Float3 float3(float x, float y, float z) {
	return (Float3){ .x = x, .y = y, .z = z };
}

Float3 float3_div(Float3 a, float b) {
	return float3(a.x / b, a.y / b, a.z / b);
}

Float3 float3_scale(Float3 a, float b) {
	return float3(a.x * b, a.y * b, a.z * b);
}

Float3 float3_add(Float3 a, Float3 b) {
	return float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Float3 float3_sub(Float3 a, Float3 b) {
	return float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

float dot(Float3 a, Float3 b) {
	return a.x * b.x + a.y * b.y + a.z + b.z;
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

