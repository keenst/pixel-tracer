[[vk::binding(0)]]
uniform RWTexture2D<float4> render_texture;
uniform RendererState renderer_state;

struct RendererState {
	float3 pixel_delta_u;
	float3 pixel_delta_v;
	float3 first_pixel_location;
	uint sample_count;
	float time;
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

static const float3 plane_origin = float3(0, -15, 0);
static const float3 plane_normal = float3(0, 1, 0);

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

	if (root < 1e-6f) {
		root = (h + sqrtd) / a;
		if (root < 1e-6f) {
			return {};
		}
	}

	float3 closest_point = origin + direction * root;

	HitInfo hit_info = {};
	hit_info.hit = true;
	hit_info.normal = (closest_point - sphere.center) / sphere.radius;
	hit_info.color = sphere.color;
	hit_info.intersect_point = closest_point;
	hit_info.t = root;
	return hit_info;
}

HitInfo ray_vs_plane(float3 origin, float3 direction) {
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
}

static float random_counter = 0;
float random() {
	return fract(sin(random_counter++ + renderer_state.time) * 1e6f);
}

float3 random_unit_vector() {
	while (true) {
		float3 rand = float3(random(), random(), random());
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

	const int sample_count = 16;

	float3 color = {};
	for (int i = 0; i < sample_count; i++) {
		float3 pixel_center = state.first_pixel_location + ((coords.x + random() - 0.5f) * state.pixel_delta_u) + ((coords.y + random() - 0.5f) * state.pixel_delta_v);

		Ray ray = {};
		ray.direction = normalize(pixel_center);
		ray.start = float3(0);

		float3[4] path_colors = {};
		int depth = 0;
		while (true) {
			if (depth >= 4) {
				path_colors[depth] = float3(0);
				break;
			}

			float closest_t = 1e20;
			HitInfo closest_hit = {};
			for (int sphere_index = 0; sphere_index < 2; sphere_index++) {
				HitInfo hit_info = ray_vs_sphere(ray.start, ray.direction, spheres[sphere_index]);
				if (hit_info.hit && hit_info.t < closest_t) {
					closest_t = hit_info.t;
					closest_hit = hit_info;
				}
			}

			if (closest_hit.hit) {
				ray.direction = random_on_hemisphere(closest_hit.normal);
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
