#include "types.slang"
#include "utilities.slang"
#include "intersections.slang"

[[vk::binding(0)]]
uniform RWTexture2D<float4> render_texture;
uniform RendererState renderer_state;
uniform StructuredBuffer<Tri> triangle_buffer;
uniform StructuredBuffer<BVHNode> bvh_buffer;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int3 coords : SV_DispatchThreadID) {
	RendererState state = renderer_state;

	random_counter = fract(sin(coords.x * coords.y) * 1e5f);

	float4x4 translation = float4x4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 2, 1
	);

	float theta = state.time;
	//float theta = 3.1415926535f;
	//float theta = 0;
	float4x4 rotation = float4x4(
		cos(theta), 0, sin(theta), 0,
		0, 1, 0, 0,
		-sin(theta), 0, cos(theta), 0,
		0, 0, 0, 1
	);

	float4x4 rotation2 = float4x4(
		cos(theta / 2), sin(theta / 2), 0, 0,
		-sin(theta / 2), cos(theta / 2), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	float4x4 transform = mul(translation, mul(rotation, rotation2));
	//float4x4 transform = translation;

	const int max_depth = 4;

	float3 color = {};
	for (int i = 0; i < state.sample_count; i++) {
		float3 pixel_center = state.first_pixel_location + ((coords.x + random() - 0.5f) * state.pixel_delta_u) + ((coords.y + random() - 0.5f) * state.pixel_delta_v);

		Ray ray = {};
		ray.direction = normalize(pixel_center);
		ray.direction = mul(float4(ray.direction, 0), transform).xyz;
		ray.start = mul(float4(0, 0, 0, 1), transform).xyz;

		float3[max_depth] path_colors = {};
		int depth = 0;
		while (true) {
			if (depth >= max_depth) {
				path_colors[depth] = float3(0);
				break;
			}

			float closest_t = 1.#INF;
			HitInfo closest_hit = {};

			// Traverse BVH
			int current_node_index = 0;
			int[64] nodes_to_visit = {};
			int to_visit_offset = 0;
			while (true) {
				BVHNode node = bvh_buffer[current_node_index];
				if (node.num_triangles > 0) {
					for (int i = 0; i < node.num_triangles; i++) {
						HitInfo hit_info = {};
						if (!ray_vs_tri(ray, triangle_buffer[node.offset + i], hit_info)) {
							continue;
						}

						if (hit_info.t < closest_t) {
							closest_t = hit_info.t;
							closest_hit = hit_info;
						}
					}

					if (to_visit_offset == 0) {
						break;
					}

					current_node_index = nodes_to_visit[--to_visit_offset];
				} else {
					int child_a_index = current_node_index + 1;
					BVHNode child_a = bvh_buffer[child_a_index];
					float3 child_a_min = float3(child_a.min_bounds[0], child_a.min_bounds[1], child_a.min_bounds[2]);
					float3 child_a_max = float3(child_a.max_bounds[0], child_a.max_bounds[1], child_a.max_bounds[2]);
					float distance_a = ray_vs_aabb_t(ray, child_a_min, child_a_max);

					int child_b_index = node.offset;
					BVHNode child_b = bvh_buffer[child_b_index];
					float3 child_b_min = float3(child_b.min_bounds[0], child_b.min_bounds[1], child_b.min_bounds[2]);
					float3 child_b_max = float3(child_b.max_bounds[0], child_b.max_bounds[1], child_b.max_bounds[2]);
					float distance_b = ray_vs_aabb_t(ray, child_b_min, child_b_max);

					if (distance_a > distance_b) {
						if (distance_a < closest_t) nodes_to_visit[to_visit_offset++] = child_a_index;
						if (distance_b < closest_t) nodes_to_visit[to_visit_offset++] = child_b_index;
					} else {
						if (distance_b < closest_t) nodes_to_visit[to_visit_offset++] = child_b_index;
						if (distance_a < closest_t) nodes_to_visit[to_visit_offset++] = child_a_index;
					}

					if (to_visit_offset == 0) {
						break;
					}

					current_node_index = nodes_to_visit[--to_visit_offset];
				}
			}

			if (closest_hit.hit) {
				ray.direction = closest_hit.normal + random_unit_vector();
				ray.start = closest_hit.intersect_point + closest_hit.normal * 1e-7f;
				//ray.start = closest_hit.intersect_point;
				path_colors[depth++] = closest_hit.color;
				continue;
			} else {
				float3 world_direction = mul(-transform, float4(ray.direction, 1)).xyz;
				float a = 0.5f * (world_direction.y + 1);
				path_colors[depth] = a * float3(0.5, 0.7, 1) + (1 - a) * float3(1, 1, 1);
				break;
			}

			break;
		}

		float3 sample_color = path_colors[depth--];
		while (depth >= 0) {
			sample_color *= path_colors[depth--];
		}

		color += sample_color / state.sample_count;
	}

	render_texture[coords.xy] = float4(color, 1);
}
