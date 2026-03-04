#include "types.slang"
#include "trace.slang"

[[vk::binding(0)]]
uniform RWTexture2D<float4> render_texture;
uniform RendererState renderer_state;
uniform StructuredBuffer<Tri> triangle_buffer;
uniform StructuredBuffer<BVHNode> bvh_buffer;
uniform StructuredBuffer<Object> object_buffer;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int3 coords : SV_DispatchThreadID) {
	RendererState state = renderer_state;

	random_counter = fract(sin(coords.x * coords.y) * 1e5f);

	const int max_depth = 4;

	// Iterate over samples
	float3 color = {};
	for (int i = 0; i < state.sample_count; i++) {
		float3 pixel_center = state.first_pixel_location + ((coords.x + random() - 0.5f) * state.pixel_delta_u) + ((coords.y + random() - 0.5f) * state.pixel_delta_v);

		Ray ray = {};
		ray.direction = normalize(pixel_center);

		// Iterate over bounces
		float3[max_depth] path_colors = {};
		int depth = 0;
		while (true) {
			if (depth >= max_depth) {
				path_colors[depth] = float3(0);
				break;
			}

			bool hit_something = false;
			HitResult closest_hit = {};
			closest_hit.t = 1.#INF;

			FOR(obj_index, state.num_objects) {
				Object object = object_buffer[obj_index];

				Ray local_ray;
				local_ray.start = mul(float4(ray.start, 1), object.inv_transform).xyz;
				local_ray.direction = mul(float4(ray.direction, 0), object.inv_transform).xyz;

				HitResult hit;
				if (!hit_mesh_bvh(local_ray, object.bvh_root_offset, object.triangle_offset, hit)) continue;

				if (hit.t > closest_hit.t) continue;

				closest_hit = hit;
				closest_hit.object = object;
				hit_something = true;
			}

			if (hit_something) {
				ray.start = mul(float4(closest_hit.ray.start, 1), closest_hit.object.transform).xyz;
				ray.direction = mul(float4(closest_hit.ray.direction, 0), closest_hit.object.transform).xyz;
				path_colors[depth] = float3(1);
				depth++;
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

		color += sample_color / state.sample_count;
	}

	render_texture[coords.xy] = float4(color, 1);
}
