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

		// Iterate over bounces
		float3[max_depth] path_colors = {};
		int depth = 0;
		while (true) {
			if (depth >= max_depth) {
				path_colors[depth - 1] = float3(0);
				break;
			}

			bool hit_something = false;
			HitResult hit = {};
			hit.t = 1.#INF;

			hit_something = hit_scene_bvh(ray, hit);

			if (hit_something) {
				Object object = object_buffer[hit.object_index];

				if (object.is_light) {
					path_colors[depth] = hit.color;
					break;
				} else {
					ray.start = mul(float4(hit.ray.start, 1), object.transform).xyz;
					ray.direction = mul(float4(hit.ray.direction, 0), mul(object.transform, transpose(object.inv_transform))).xyz;

					path_colors[depth] = hit.color;

					depth++;
				}
			} else {
				float a = 0.5f * (ray.direction.y + 1);
				//path_colors[depth] = a * float3(0.5, 0.7, 1) + (1 - a) * float3(1, 1, 1);
				break;
			}
		}

		float3 sample_color = path_colors[depth--];
		while (depth >= 0) {
			sample_color *= path_colors[depth--];
		}

		color += sample_color / state.sample_count;
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
