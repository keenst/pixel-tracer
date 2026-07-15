#include "types.slang"
#include "trace.slang"

[[vk::binding(0)]]
uniform RWTexture2D<float4> render_texture;
uniform RendererState renderer_state;
uniform StructuredBuffer<Tri> triangle_buffer;
uniform StructuredBuffer<BVHNode> mesh_bvh_buffer;
uniform StructuredBuffer<Object> object_buffer;
uniform StructuredBuffer<uint32_t> object_index_buffer;
uniform StructuredBuffer<BVHNode> scene_bvh_buffer;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int3 coords : SV_DispatchThreadID) {
	RendererState state = renderer_state;

	global_random_seed = float2(coords.x, coords.y) * state.time;
	random_counter = coords.x * coords.y * state.time;

	const int max_depth = 4;

	// Iterate over samples
	float3 color = {};
	for (int i = 0; i < state.sample_count; i++) {
		float3 pixel_center = state.first_pixel_location + ((coords.x + random() - 0.5f) * state.pixel_delta_u) + ((coords.y + random() - 0.5f) * state.pixel_delta_v);

		Ray ray = {};
		ray.direction = normalize(pixel_center);

		ray.start = mul(float4(ray.start, 1), state.camera_transform).xyz;
		ray.direction = mul(float4(ray.direction, 0), state.camera_transform).xyz;

		float3 sample_color = 1;
		float3 result_color = 0;

		// Iterate over bounces
		int depth = 0;
		while (true) {
			if (depth >= max_depth) {
				break;
			}

			HitResult hit = {};
			hit.t = 1.#INF;

			bool hit_something = hit_scene_bvh(ray, hit);

			if (!hit_something) {
				break;
			}

			Object object = object_buffer[hit.object_index];

			if (object.is_light) {
				//sample_color *= object.material.color;
				result_color += sample_color * object.material.color;
				break;
			}

			float3 sampled_direction = sample_phong(ray.direction, hit.normal, object.material);
			float3 brdf = brdf_phong(ray.direction, hit.normal, sampled_direction, object.material);

			// Light sampling
			float3 light_normal;
			float3 light_color;
			float light_pdf;

			float3 point_on_light = sample_random_point_on_random_light(light_normal, light_pdf, light_color);
			float3 light_incoming = normalize(point_on_light - hit.point_hit);

			Ray direct_light_sampling_ray;
			direct_light_sampling_ray.start = hit.point_hit;
			direct_light_sampling_ray.direction = light_incoming;

			HitResult direct_light_sampling_hit = {};
			if (hit_scene_bvh(direct_light_sampling_ray, direct_light_sampling_hit)) {
				//float3 hit_position = direct_light_sampling_ray.start + direct_light_sampling_ray.direction * direct_light_sampling_hit.t;
				float3 hit_position = direct_light_sampling_hit.point_hit;
				if (distance(hit_position, point_on_light) < 1e-5f) {
					float distance_squared = dot(point_on_light - hit.point_hit, point_on_light - hit.point_hit);
					float g = abs(-dot(light_normal, light_incoming)) / distance_squared;

					//sample_color *= brdf_phong(ray.direction, hit.normal, sampled_direction, object.material);

					result_color += sample_color * brdf * g * saturate(dot(hit.normal, light_incoming)) * light_color / light_pdf;
					//result_color += sample_color;
				}
			}

			sample_color *= brdf;

			// Material sampling
			ray.start = hit.point_hit;
			ray.direction = sampled_direction;

			depth++;
#if 0
			bool hit_something = false;
			HitResult hit = {};
			hit.t = 1.#INF;

			hit_something = hit_scene_bvh(ray, hit);

			if (hit_something) {
				Object object = object_buffer[hit.object_index];

				if (object.is_light) {
					sample_color *= hit.color;
					break;
				} else {
					Ray old_ray = ray;

					ray.start = mul(float4(hit.ray.start, 1), object.transform).xyz;
					ray.direction = mul(float4(hit.ray.direction, 0), mul(object.transform, transpose(object.inv_transform))).xyz;

					// Direct light sampling
					float light_prob;
					float3 light_normal;
					float3 light_color;
					float3 point_on_light = sample_random_point_on_random_light(light_normal, light_prob, light_color);

					Ray direct_light_sampling_ray;
					direct_light_sampling_ray.start = ray.start;
					direct_light_sampling_ray.direction = normalize(point_on_light - ray.start);

					float direct_light_sampling_factor = 1.0f;

					HitResult direct_light_sampling_hit = {};
					if (hit_scene_bvh(direct_light_sampling_ray, direct_light_sampling_hit)) {
						//float3 hit_position = direct_light_sampling_ray.start + direct_light_sampling_ray.direction * direct_light_sampling_hit.t;
						float3 hit_position = direct_light_sampling_hit.point_hit;
						if (distance(hit_position, point_on_light) > 1e-5f) {
							direct_light_sampling_factor = 0.0f;
						}
					} else {
						direct_light_sampling_factor = 0.0f;
					}

					//light_prob = pdf_phong(old_ray.direction, hit.normal, ray.direction, object.material);

					float dot_a = max(0.001f, abs(dot(light_normal, normalize(point_on_light - direct_light_sampling_ray.start))));
					float dot_b = max(0.001f, abs(dot(hit.normal,   normalize(point_on_light - direct_light_sampling_ray.start))));
					float g = 1.0f / (length(direct_light_sampling_ray.start - point_on_light) * length(direct_light_sampling_ray.start - point_on_light)) * dot_a * dot_b;

					//path_colors[depth] = hit.color;// / pdf_phong(old_ray.direction, hit.normal, ray.direction, object.material);
					direct_color += sample_color * hit.color * light_color * g * direct_light_sampling_factor / light_prob;
					sample_color *= hit.color;// / pdf_phong(old_ray.direction, hit.normal, ray.direction, object.material);
					//direct_color += light_color * g * direct_light_sampling_factor / light_prob;

					depth++;
				}
			} else {
				float a = 0.5f * (ray.direction.y + 1);
				sample_color = 0;
				//path_colors[depth] = a * float3(0.5, 0.7, 1) + (1 - a) * float3(1, 1, 1);
				break;
			}
#endif
		}

#if 0
		if (any(isnan(sample_color))) {
			color += sample_color / state.sample_count;
		} else {
			color = 0;
		}
#else
		color += result_color / state.sample_count;
#endif

		//color += (direct_color) / state.sample_count / 2;
		//color += direct_color / state.sample_count;
	}

	if (state.progressive) {
		float4 tex_color = render_texture[coords.xy];

		/*
		float4 current_color = tex_color * ((float)state.num_frames / (state.num_frames + 1));
		current_color += float4(color, 1) * (1.0f / (state.num_frames + 1));

		render_texture[coords.xy] = current_color;
		*/
		render_texture[coords.xy] = tex_color + (float4(color, 1) - tex_color) / state.num_frames;
	} else {
		render_texture[coords.xy] = float4(color, 1);
	}
}
