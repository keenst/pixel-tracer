#ifdef DEBUG
#define IN_DEBUG(a) a
#else
#define IN_DEBUG(a)
#endif

#define FOR(i, n) for (uint i = 0; i < n; i++)
#define FLOAT3(arr) float3(arr[0], arr[1], arr[2])

static float pi = 3.1415926535897932384626;

static float random_counter = 0;
static float2 global_random_seed = { 0, 0 };

float random() {
	//return fract(sin(random_counter++ + renderer_state.time) * 1e7f);
	//return fract(sin(dot(float2(random_counter++ / 100, (random_counter + 31) / 100), float2(12.9898, 78.233))) * 43758.5453);
	global_random_seed.x +=  7.0f / 1000;
	global_random_seed.y += 31.0f / 1000;
	return fract(sin(dot(global_random_seed, float2(12.9898, 78.233))) * 43758.5453);
}

/*
float random(float2 st) {
	return fract(sin(dot(st, float2(12.9898, 78.233))) * 43758.5453);
}
*/

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
	if (dot(vector, normal) > 0) {
		return vector;
	} else {
		return -vector;
	}
}

float3 vector_from_angles(float theta, float phi) {
	return float3(
			sin(theta) * cos(phi),
			sin(theta) * sin(phi),
			cos(theta)
	);
}

#if 0
float3 brdf_phong(float3 outgoing, float3 normal, Material material) {
	float diffuse_factor = material.roughness; // k_d
	float specular_factor = 1 - diffuse_factor; // k_s

	float3 reflection_axis = normalize(reflect(outgoing, normal));

	/* Meant to match Sebastian Lagues specular model.
	 * The original takes the smoothness factor from the material but I just hardcoded it for now.
	 * Taken from this repo:
	 * https://github.com/ComputationallyBased/RayTracingMIS/blob/main/Assets/Scripts/Shaders/RayTracer.shader
	 */
	float n = (2.0f / (pow(0.0f, 3.0f))) - 2.0f; 

	float3 incoming;
	if (random() < diffuse_factor) {
		incoming = random_on_hemisphere(normal);
	} else {
		float rand1 = random();
		float rand2 = random();

		float cos_alpha = pow(rand1, 1.0f / (n + 1.0f));
		float sin_alpha = sqrt(max(0.0f, 1.0f - cos_alpha * cos_alpha));
		float phi		= 2.0f * pi * rand2;

		float3 local = float3(cos(phi) * sin_alpha, sin(phi) * sin_alpha, cos_alpha);
		incoming = transform_basis(local, reflection_axis);
	}

	return incoming;
}

float pdf_phong(float3 outgoing, float3 normal, float3 incoming, Material material) {
	float diffuse_factor = material.roughness; // k_d
	float specular_factor = 1 - diffuse_factor; // k_s

	float3 reflection_axis = normalize(reflect(outgoing, normal));

	/* Meant to match Sebastian Lagues specular model.
	 * The original takes the smoothness factor from the material but I just hardcoded it for now.
	 * Taken from this repo:
	 * https://github.com/ComputationallyBased/RayTracingMIS/blob/main/Assets/Scripts/Shaders/RayTracer.shader
	 */
	float n = (2.0f / (pow(0.0f, 3.0f))) - 2.0f; 

	float prob_diffuse = 1.0f / (4.0f * pi);
	float prob_specular = ((n + 1) / (2.0f * pi)) * pow(max(dot(incoming, reflection_axis), 0.001f), n);

	return diffuse_factor * prob_diffuse + specular_factor * prob_specular;
}
#endif

float3 brdf_phong(float3 outgoing, float3 normal, float3 incoming, Material material) {
	float3 reflection_axis = reflect(outgoing, normal);
	float cos_alpha = saturate(dot(reflection_axis, outgoing));
	float cos_theta = saturate(dot(reflection_axis, incoming));

	float specular_exponent = 8;

	float specular_part = ((specular_exponent + 2) / (2 * pi)) * pow(cos_alpha, specular_exponent);
	float diffuse_part = 1.0f / pi;

	float specular_factor = 1 - material.roughness;
	float diffuse_factor = material.roughness;

	// TODO(leo): This is theoretically the correct one, doesn't work currently tho.
	//float specular_pdf = ((specular_exponent + 1) / (2 * pi)) * pow(cos_alpha, specular_exponent);
	float specular_pdf = ((specular_exponent + 1) / (2 * pi));
	float diffuse_pdf = (1.0f / pi) * cos_theta;
	specular_pdf = 1;
	diffuse_pdf = 1;

	specular_pdf = max(1e-5f, specular_pdf);
	diffuse_pdf = max(1e-5f, diffuse_pdf);

	float brdf = specular_factor * specular_part / specular_pdf + diffuse_factor * diffuse_part / diffuse_pdf;
	//float brdf = specular_factor * specular_part + diffuse_factor * diffuse_part;

	return material.color * brdf;// / pdf;
}

float pdf_phong(float3 outgoing, float3 normal) {
	float3 reflection_axis = reflect(outgoing, normal);
	float cos_alpha = dot(reflection_axis, outgoing);

	float specular_exponent = 8;

	return ((specular_exponent + 1) / (2 * pi)) * pow(cos_alpha, specular_exponent);
}

float3 sample_phong(float3 outgoing, float3 normal, Material material) {
	if (random() < material.roughness) {
		return random_on_hemisphere(normal);
	} else {
		return reflect(outgoing, normal);
	}
}

float3 sample_random_point_on_random_light(out float3 normal, out float pdf, out float3 light_color) {
	int light_index = (int)random(0, renderer_state.num_lights);
	Object light = object_buffer[light_index];

	int tri_index = (int)random(light.triangle_offset, light.triangle_offset + light.num_triangles);
	Tri tri = triangle_buffer[tri_index];

	float3 a = mul(float4(tri.verts[0].position, 1.0f), light.transform).xyz;
	float3 b = mul(float4(tri.verts[1].position, 1.0f), light.transform).xyz;
	float3 c = mul(float4(tri.verts[2].position, 1.0f), light.transform).xyz;

	float u = random();
	float v = random();

	if (u + v > 1) {
		u = 1 - u;
		v = 1 - v;
	}

	float3 sampled_point = u * a + v * b + (1 - u - v) * c;

	normal = u * tri.verts[0].normal + v * tri.verts[1].normal + (1 - u - v) * tri.verts[2].normal;
	normal = normalize(mul(float4(normal, 0), mul(light.transform, transpose(light.inv_transform))).xyz);

	float area = 0.5f * length(cross(a - b, a - c));

	pdf =  1.0f / (float)renderer_state.num_lights;
	pdf *= 1.0f / (float)light.num_triangles;
	pdf *= 1.0f / area;

	light_color = light.material.color;

	return sampled_point;
}

float3 transform_basis(float3 local, float3 normal) {
	float3 up = abs(normal.x) > 0.9f ? float3(0, 1, 0) : float3(1, 0, 0);
	float3 tangent = normalize(cross(normal, up));
	float3 bitangent = cross(normal, tangent);

	return
		local.x * bitangent +
		local.y * tangent +
		local.z * normal;
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
	hit_info.t = t;
	hit_info.intersect_point = intersection_point;
	hit_info.tri = tri;
	hit_info.u = u;
	hit_info.v = v;
	return true;
}

bool hit_scene_bvh(
		Ray ray,
		out HitResult out_result,
		IN_DEBUG(inout RayDebugInfo ray_debug_info))
{
	float closest_t = 1.#INF;
	HitResult closest_result = {};

	bool hit_something = false;

	int current_node_index = 0;
	int[64] nodes_to_visit = {};
	int to_visit_offset = 0;
	while (true) {
		BVHNode node = scene_bvh_buffer[current_node_index];

#ifdef DEBUG
		if (ray_vs_aabb_t(ray, FLOAT3(node.min_bounds), FLOAT3(node.max_bounds)) != 1.#INF) {
			ray_debug_info.scene_nodes++;
		}
#endif

		if (node.num_primitives > 0) { // Is leaf node, test object
			FOR(i, node.num_primitives) {
				Object object = object_buffer[object_index_buffer[node.offset + i]];

				Ray local_ray;
				local_ray.start = mul(float4(ray.start, 1), object.inv_transform).xyz;
				local_ray.direction = mul(float4(ray.direction, 0), object.inv_transform).xyz;

				float incoming_length = length(local_ray.direction);

				HitResult mesh_result = {};
				if (!hit_mesh_bvh(local_ray, object_index_buffer[node.offset + i], mesh_result, IN_DEBUG(ray_debug_info))) {
					continue;
				}

				hit_something = true;

				// TODO: I think there could be something happening with t not being adjusted
				// for scaled spaces. Poke around with this
				//mesh_result.t /= incoming_length;
				if (mesh_result.t < closest_t) {
					closest_t = mesh_result.t;
					closest_result = mesh_result;
				}
			}

			if (to_visit_offset == 0) break;
			current_node_index = nodes_to_visit[--to_visit_offset];
		} else { // Isn't leaf node, add children to queue
			int child_a_index = current_node_index + 1;
			BVHNode child_a = scene_bvh_buffer[child_a_index];
			float3 child_a_min = float3(child_a.min_bounds[0], child_a.min_bounds[1], child_a.min_bounds[2]);
			float3 child_a_max = float3(child_a.max_bounds[0], child_a.max_bounds[1], child_a.max_bounds[2]);
			float distance_a = ray_vs_aabb_t(ray, child_a_min, child_a_max);

			int child_b_index = node.offset;
			BVHNode child_b = scene_bvh_buffer[child_b_index];
			float3 child_b_min = float3(child_b.min_bounds[0], child_b.min_bounds[1], child_b.min_bounds[2]);
			float3 child_b_max = float3(child_b.max_bounds[0], child_b.max_bounds[1], child_b.max_bounds[2]);
			float distance_b = ray_vs_aabb_t(ray, child_b_min, child_b_max);

			IN_DEBUG(ray_debug_info.scene_aabb_tests += 2);

#if 0
			if (distance_a > distance_b) {
				nodes_to_visit[to_visit_offset++] = child_a_index;
				nodes_to_visit[to_visit_offset++] = child_b_index;
			} else {
				nodes_to_visit[to_visit_offset++] = child_b_index;
				nodes_to_visit[to_visit_offset++] = child_a_index;
			}
#else
			if (distance_a > distance_b) {
				if (distance_a < closest_t) nodes_to_visit[to_visit_offset++] = child_a_index;
				if (distance_b < closest_t) nodes_to_visit[to_visit_offset++] = child_b_index;
			} else {
				if (distance_b < closest_t) nodes_to_visit[to_visit_offset++] = child_b_index;
				if (distance_a < closest_t) nodes_to_visit[to_visit_offset++] = child_a_index;
			}
#endif

			if (to_visit_offset == 0) break;
			current_node_index = nodes_to_visit[--to_visit_offset];
		}
	}

	if (hit_something) {
		out_result = closest_result;
		return true;
	}

	out_result = {};
	return false;
}

bool hit_mesh_bvh(
		Ray ray,
		uint object_index,
		out HitResult out_result,
		IN_DEBUG(inout RayDebugInfo ray_debug_info))
{
	float closest_t = 1.#INF;
	HitInfo closest_hit = {};

	Object object = object_buffer[object_index];
	uint bvh_offset = object.bvh_root_offset;
	uint triangle_offset = object.triangle_offset;

	int current_node_index = 0;
	int[64] nodes_to_visit = {};
	int to_visit_offset = 0;
	while (true) {
		BVHNode node = mesh_bvh_buffer[current_node_index + bvh_offset];

		if (node.num_primitives > 0) { // Is leaf node, test triangles
			for (int i = 0; i < node.num_primitives; i++) {
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
			BVHNode child_a = mesh_bvh_buffer[child_a_index + bvh_offset];
			float3 child_a_min = float3(child_a.min_bounds[0], child_a.min_bounds[1], child_a.min_bounds[2]);
			float3 child_a_max = float3(child_a.max_bounds[0], child_a.max_bounds[1], child_a.max_bounds[2]);
			float distance_a = ray_vs_aabb_t(ray, child_a_min, child_a_max);

			int child_b_index = node.offset;
			BVHNode child_b = mesh_bvh_buffer[child_b_index + bvh_offset];
			float3 child_b_min = float3(child_b.min_bounds[0], child_b.min_bounds[1], child_b.min_bounds[2]);
			float3 child_b_max = float3(child_b.max_bounds[0], child_b.max_bounds[1], child_b.max_bounds[2]);
			float distance_b = ray_vs_aabb_t(ray, child_b_min, child_b_max);

			IN_DEBUG(ray_debug_info.mesh_aabb_tests += 2);

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
		Tri tri = closest_hit.tri;
		float u = closest_hit.u;
		float v = closest_hit.v;
		float w = 1 - u - v;
		float3 normal = u * tri.verts[0].normal + v * tri.verts[1].normal + w * tri.verts[2].normal;

		if (dot(ray.direction, normal) > 0) {
			out_result = {};
			return false;
		}

		out_result = {};

		out_result.ray.direction = sample_phong(ray.direction, normal, object.material);

		out_result.color = object.material.color;

		out_result.ray.start = closest_hit.intersect_point + normal * 1e-6f;
		out_result.t = closest_hit.t;
		out_result.normal = normal;
		out_result.point_hit = mul(float4(ray.start + ray.direction * closest_hit.t, 1), object.transform).xyz;

		/*
		out_result.u = closest_hit.u;
		out_result.v = closest_hit.v;
		*/
		out_result.object_index = object_index;
		return true;
	}

	out_result = {};
	return false;
}
