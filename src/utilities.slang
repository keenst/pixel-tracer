static float random_counter = 0;
float random() {
	return fract(sin(random_counter++ + renderer_state.time) * 1e5f);
}

float random(float from, float to) {
	return from + (to - from) * random();
}

float3 random_unit_vector() {
	while (true) {
		float3 rand = float3(random(-1, 1), random(-1, 1), random(-1, 1));
		float length_squared = dot(rand, rand);
		if (length_squared > 1e-15 && length_squared <= 1) {
			return rand / sqrt(length_squared);
		}
	}
}

float3 random_on_hemisphere(float3 normal) {
	float3 vector = random_unit_vector();
	if (dot(vector, normal) >= 0) {
		return vector;
	} else {
		return -vector;
	}
}
