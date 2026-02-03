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

bool new_ray_vs_aabb(Ray ray, float3 aabb_min, float3 aabb_max) {
	const float eps = 1e-10f;

	float3 p0 = ray.start;
	float3 p1 = ray.start + ray.direction * 1e5;

	float3 c = (aabb_min + aabb_max) * 0.5f;
	float3 e = aabb_max - c;

	float3 m = (p0 + p1) * 0.5f;
	float3 d = p1 - m;
	m = m - c;

	float adx = abs(d.x);
	if (abs(m.x) > e.x + adx) return false;
	float ady = abs(d.y);
	if (abs(m.y) > e.y + ady) return false;
	float adz = abs(d.z);
	if (abs(m.z) > e.z + adz) return false;

	adx += eps;
	ady += eps;
	adz += eps;

	if (abs(m.y * d.z - m.z * d.y) > e.y * adz + e.z * ady) return false;
	if (abs(m.z * d.x - m.x * d.z) > e.x * adz + e.z * adx) return false;
	if (abs(m.x * d.y - m.y * d.x) > e.x * ady + e.y * adx) return false;

	return true;
}

bool ray_vs_tri(Ray ray, Tri tri, out HitInfo hit_info) {
	const float eps = 1e-10f;

	hit_info = {};

	float3 v0 = tri.verts[0];
	float3 v1 = tri.verts[1];
	float3 v2 = tri.verts[2];

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

bool new_ray_vs_tri(Ray ray, Tri tri, out HitInfo hit_info) {
	const float eps = 1e-10f;

	float3 edge1 = tri.verts[1] - tri.verts[0];
	float3 edge2 = tri.verts[2] - tri.verts[0];
	float3 ray_cross_e2 = cross(ray.direction, edge2);
	float det = dot(edge1, ray_cross_e2);

	if (det > -eps && det < eps) {
		hit_info = {};
		return false;
	}

	float inv_det = 1.0f / det;
	float3 s = ray.start - tri.verts[0];
	float u = inv_det * dot(s, ray_cross_e2);

	if ((u < 0 && abs(u) > eps) || (u > 1 && abs(u - 1) > eps)) {
		hit_info = {};
		return false;
	}

	float3 s_cross_e1 = cross(s, edge1);
	float v = inv_det * dot(ray.direction, s_cross_e1);

	if ((u < 0 && abs(v) > eps) || (u + v > 1 && abs(u + v - 1) > eps)) {
		hit_info = {};
		return false;
	}

	float t = inv_det * dot(edge2, s_cross_e1);

	if (t > eps) {
		hit_info.hit = true;
		hit_info.normal = normalize(cross(edge1, edge2));
		hit_info.color = float3(1, 1, 1);
		hit_info.intersect_point = ray.start + ray.direction * t;
		hit_info.t = t;
		return true;
	} else {
		hit_info = {};
		return false;
	}
}
