[[vk::binding(0)]]
uniform RWTexture2D<float4> render_texture;
uniform RendererState renderer_state;
uniform StructuredBuffer<float3> vertex_buffer;

struct RendererState {
	float3 pixel_delta_u;
	float3 pixel_delta_v;
	float3 first_pixel_location;
	uint sample_count;
	float time;
	uint vertex_count;
}

struct Sphere {
	float3 center;
	float radius;
	float3 color;
}

static const Sphere sphere_a = { float3(0, 0, -10), 5, float3(1.0f, 0.0f, 0.0f) };
static const Sphere sphere_b = { float3(0, -1005, -10), 1000, float3(0.5f) };

static const Sphere[] spheres = {
	sphere_a,
	sphere_b
};

struct Tri {
	float3[3] verts;
};

static const Tri tri = { { float3(-0.5, -0.5, -1), float3(0.5, -0.5, -1), float3(0, 0.5, -1) } };
static const Tri tri_2 = { { float3(0, 0.5, -1), float3(-0.5, -0.5, -1), float3(-1, 0.5, -3) } };
static const Tri tri_3 = { { float3(0, 0.5, -1), float3(0.5, -0.5, -1), float3(1, 0.5, -3) } };

static const Tri[] tris = {
	tri,
	tri_2,
	tri_3
};

struct Ray {
	float3 start;
	float3 direction;
};

struct HitInfo {
	bool hit;
	float t;
	float3 intersect_point;
	float3 normal;
	float3 color;
};

HitInfo ray_vs_tri(Ray ray, Tri tri) {
	const float eps = 1e-10f;

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
		return {};
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
	if (dot(normal, C) < 0) return {};

	// Calculate v
	float3 v2p = intersection_point - v2;
	float3 v2v0 = v0 - v2;
	C = cross(v2v0, v2p);
	float v = (length(C) / 2) / area;
	if (dot(normal, C) < 0) return {};

	// Third edge
	float3 v0p = intersection_point - v0;
	C = cross(v0v1, v0p);
	if (dot(normal, C) < 0) return {};

	// Return
	HitInfo hit_info;
	hit_info.hit = true;
	hit_info.normal = normal;
	hit_info.color = float3(1, 1, 1);
	hit_info.intersect_point = intersection_point;
	hit_info.t = t;
	return hit_info;
}

HitInfo ray_vs_sphere(float3 origin, float3 direction, Sphere sphere) {
	float3 oc = sphere.center - origin;
	float a = dot(direction, direction);
	float h = dot(direction, oc);
	float c = dot(oc, oc) - sphere.radius * sphere.radius;
	float discriminant = h * h - a * c;

	if (discriminant < 0) {
		return {};
	}

	float sqrtd = sqrt(discriminant);
	float root = (h - sqrtd) / a;

	if (root < 1e-10f) {
		root = (h + sqrtd) / a;
		if (root < 1e-10f) {
			return {};
		}
	}

	float3 closest_point = origin + direction * root;
	float3 normal = (closest_point - sphere.center) / sphere.radius;

	if (dot(normal, direction) > 0) {
		return {};
	}

	HitInfo hit_info = {};
	hit_info.hit = true;
	hit_info.normal = normal;
	hit_info.color = sphere.color;
	hit_info.intersect_point = closest_point;
	hit_info.t = root;
	return hit_info;
}

HitInfo ray_vs_plane(float3 origin, float3 direction) {
/*
	float d = dot(plane_origin, plane_normal);
	float t = (d - dot(origin, plane_normal)) / dot(direction, plane_normal);

	if (t < 1e-2f) {
		return {};
	}

	HitInfo hit_info = {};
	hit_info.hit = true;
	hit_info.normal = plane_normal;
	hit_info.color = float3(0, 0, 1);
	hit_info.intersect_point = origin + direction * t;
	hit_info.t = t;
	return hit_info;
*/
	return {};
}

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

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int3 coords : SV_DispatchThreadID) {
	RendererState state = renderer_state;

	random_counter = fract(sin(coords.x * coords.y) * 1e5f);

	const int sample_count = 16;
	const int max_depth = 4;

	float3 color = {};
	for (int i = 0; i < sample_count; i++) {
		float3 pixel_center = state.first_pixel_location + ((coords.x + random() - 0.5f) * state.pixel_delta_u) + ((coords.y + random() - 0.5f) * state.pixel_delta_v);

		Ray ray = {};
		ray.direction = normalize(pixel_center);
		ray.start = float3(0);

		float3[max_depth] path_colors = {};
		int depth = 0;
		while (true) {
			if (depth >= max_depth) {
				path_colors[depth] = float3(0);
				break;
			}

			float closest_t = 1e20;
			HitInfo closest_hit = {};
			/*
			for (int tri_index = 0; tri_index < 3; tri_index++) {
				HitInfo hit_info = ray_vs_tri(ray, tris[tri_index]);
				if (hit_info.hit && hit_info.t < closest_t) {
					closest_t = hit_info.t;
					closest_hit = hit_info;
				}
			}
			*/

			const int tri_count = state.vertex_count / 3;
			for (int tri_index = 0; tri_index < tri_count; tri_index++) {
				int start = tri_index * 9;
				Tri tri = {
				/*
					float3(vertex_buffer[start], vertex_buffer[start + 1], vertex_buffer[start + 2]),
					float3(vertex_buffer[start + 3], vertex_buffer[start + 4], vertex_buffer[start + 5]),
					float3(vertex_buffer[start + 6], vertex_buffer[start + 7], vertex_buffer[start + 8])
					*/
					vertex_buffer[tri_index * 3],
					vertex_buffer[tri_index * 3 + 1],
					vertex_buffer[tri_index * 3 + 2]
				};

				HitInfo hit_info = ray_vs_tri(ray, tri);
				if (hit_info.hit && hit_info.t < closest_t) {
					closest_t = hit_info.t;
					closest_hit = hit_info;
				}
			}

			if (closest_hit.hit) {
				ray.direction = closest_hit.normal + random_unit_vector();
				ray.start = closest_hit.intersect_point;
				path_colors[depth++] = closest_hit.color;
				continue;
			} else {
				float a = 0.5f * (ray.direction.y + 1);
				path_colors[depth] = a * float3(0.5, 0.7, 1) + (1 - a) * float3(1, 1, 1);
				break;
			}
		}

		float3 sample_color = path_colors[depth--];
		while (depth >= 0) {
			sample_color *= path_colors[depth--];
		}

		color += sample_color / sample_count;
	}

	render_texture[coords.xy] = float4(color, 1);
}
