#ifdef DEBUG
#define IN_DEBUG(a) a
#else
#define IN_DEBUG(a)
#endif

#define FOR(i, n) for (uint i = 0; i < n; i++)
#define FLOAT3(arr) float3(arr[0], arr[1], arr[2])

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

bool ray_vs_aabb(Ray ray, float3 aabb_min, float3 aabb_max, out HitInfo hit_info) {
	const float eps = 1e-10f;

	float t_min = 0.0f;
	float t_max = 1e20f;

	// Iterate over each axis
	for (int i = 0; i < 3; i++) {
		// Check if ray is parallel to slab
		if (abs(ray.direction[i]) < eps) {
			// Check if origin is outside the slab
			if (ray.start[i] < aabb_min[i] || ray.start[i] > aabb_max[i]) {
				hit_info = {};
				return false;
			}
		} else {
			// Intersection t-value for far and near planes of slab
			float dir_inv = 1.0f / ray.direction[i];
			float t_near = (aabb_min[i] - ray.start[i]) * dir_inv;
			float t_far = (aabb_max[i] - ray.start[i]) * dir_inv;

			if (t_near > t_far) {
				float temp = t_near;
				t_near = t_far;
				t_far = temp;
			}

			t_min = max(t_min, t_near);
			t_max = min(t_max, t_far);

			if (t_min > t_max) {
				hit_info = {};
				return false;
			}
		}
	}

	hit_info.hit = true;
	hit_info.color = float3(1, 1, 1);
	hit_info.t = t_min;
	return true;
}

float ray_vs_aabb_t(Ray ray, float3 aabb_min, float3 aabb_max) {
	const float eps = 1e-10f;

	float t_min = 0.0f;
	float t_max = 1.#INF;

	// Iterate over each axis
	for (int i = 0; i < 3; i++) {
		// Check if ray is parallel to slab
		if (abs(ray.direction[i]) < eps) {
			// Check if origin is outside the slab
			if (ray.start[i] < aabb_min[i] || ray.start[i] > aabb_max[i]) {
				return 1.#INF;
			}
		} else {
			// Intersection t-value for far and near planes of slab
			float dir_inv = 1.0f / ray.direction[i];
			float t_near = (aabb_min[i] - ray.start[i]) * dir_inv;
			float t_far = (aabb_max[i] - ray.start[i]) * dir_inv;

			if (t_near > t_far) {
				float temp = t_near;
				t_near = t_far;
				t_far = temp;
			}

			t_min = max(t_min, t_near);
			t_max = min(t_max, t_far);

			if (t_min > t_max) {
				return 1.#INF;
			}
		}
	}

	return t_min;
}

bool ray_vs_tri(Ray ray, Tri tri, out HitInfo hit_info) {
	const float eps = 1e-10f;

	hit_info = {};

	float3 v0 = tri.verts[0].position;
	float3 v1 = tri.verts[1].position;
	float3 v2 = tri.verts[2].position;

	float3 v0v1 = v1 - v0;
	float3 v0v2 = v2 - v0;

	float3 normal = cross(v0v1, v0v2);

	// Plane intersection
	float d = dot(v0, normal);
	float t = (d - dot(ray.start, normal)) / dot(ray.direction, normal);

	if (t < eps) {
		return false;
	}

	float3 intersection_point = ray.start + ray.direction * t;

	// In-triangle test
	float area = length(normal) / 2;
	float3 C;

	// Calculate u
	float3 v1p = intersection_point - v1;
	float3 v1v2 = v2 - v1;
	C = cross(v1v2, v1p);
	float u = (length(C) / 2) / area;
	if (dot(normal, C) < 0) return false;

	// Calculate v
	float3 v2p = intersection_point - v2;
	float3 v2v0 = v0 - v2;
	C = cross(v2v0, v2p);
	float v = (length(C) / 2) / area;
	if (dot(normal, C) < 0) return false;

	// Third edge
	float3 v0p = intersection_point - v0;
	C = cross(v0v1, v0p);
	if (dot(normal, C) < 0) return false;

	// Return
	hit_info.hit = true;
	hit_info.normal = normalize(normal);
	hit_info.color = float3(1, 1, 1);
	hit_info.intersect_point = intersection_point;
	hit_info.t = t;
	return true;
}

bool hit_mesh_bvh(
		Ray ray,
		uint bvh_offset,
		uint triangle_offset,
		out HitResult out_result,
		IN_DEBUG(inout RayDebugInfo ray_debug_info)) {
	float closest_t = 1.#INF;
	HitInfo closest_hit = {};

	int current_node_index = 0;
	int[64] nodes_to_visit = {};
	int to_visit_offset = 0;
	while (true) {
		BVHNode node = bvh_buffer[current_node_index + bvh_offset];
		if (node.num_triangles > 0) { // Is leaf node, test triangles
			for (int i = 0; i < node.num_triangles; i++) {
				IN_DEBUG(ray_debug_info.tri_tests++);

				HitInfo hit_info = {};
				if (!ray_vs_tri(ray, triangle_buffer[node.offset + triangle_offset + i], hit_info)) {
					continue;
				}

				if (hit_info.t < closest_t) {
					closest_t = hit_info.t;
					closest_hit = hit_info;
				}
			}

			if (to_visit_offset == 0) break;
			current_node_index = nodes_to_visit[--to_visit_offset];
		} else { // Isn't leaf node, add children to queue
			int child_a_index = current_node_index + 1;
			BVHNode child_a = bvh_buffer[child_a_index + bvh_offset];
			float3 child_a_min = float3(child_a.min_bounds[0], child_a.min_bounds[1], child_a.min_bounds[2]);
			float3 child_a_max = float3(child_a.max_bounds[0], child_a.max_bounds[1], child_a.max_bounds[2]);
			float distance_a = ray_vs_aabb_t(ray, child_a_min, child_a_max);

			int child_b_index = node.offset;
			BVHNode child_b = bvh_buffer[child_b_index + bvh_offset];
			float3 child_b_min = float3(child_b.min_bounds[0], child_b.min_bounds[1], child_b.min_bounds[2]);
			float3 child_b_max = float3(child_b.max_bounds[0], child_b.max_bounds[1], child_b.max_bounds[2]);
			float distance_b = ray_vs_aabb_t(ray, child_b_min, child_b_max);

			IN_DEBUG(ray_debug_info.aabb_tests += 2);

			if (distance_a > distance_b) {
				if (distance_a < closest_t) nodes_to_visit[to_visit_offset++] = child_a_index;
				if (distance_b < closest_t) nodes_to_visit[to_visit_offset++] = child_b_index;
			} else {
				if (distance_b < closest_t) nodes_to_visit[to_visit_offset++] = child_b_index;
				if (distance_a < closest_t) nodes_to_visit[to_visit_offset++] = child_a_index;
			}

			if (to_visit_offset == 0) break;
			current_node_index = nodes_to_visit[--to_visit_offset];
		}
	}

	if (closest_hit.hit) {
		out_result = {};
		out_result.ray.direction = closest_hit.normal + random_unit_vector();
		out_result.ray.start = closest_hit.intersect_point + closest_hit.normal * 1e-6f;
		out_result.t = closest_hit.t;
		return true;
	}

	out_result = {};
	return false;
}
