#define DEBUG

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

	random_counter = fract(sin(coords.x * coords.y) * 1e5f);

	float3 pixel_center = state.first_pixel_location + (coords.x - 0.5f) * state.pixel_delta_u + (coords.y - 0.5f) * state.pixel_delta_v;

	Ray ray = {};
	ray.direction = normalize(pixel_center);

	ray.start = mul(float4(ray.start, 1), state.camera_transform).xyz;
	ray.direction = mul(float4(ray.direction, 0), state.camera_transform).xyz;

	RayDebugInfo ray_debug_info = {};

	bool hit_something = false;
	HitResult closest_hit = {};
	closest_hit.t = 1.#INF;

	hit_something = hit_scene_bvh(ray, closest_hit, ray_debug_info);

#if 0
	FOR(object_index, state.num_objects) {
		Object object = object_buffer[object_index];

		Ray local_ray;
		local_ray.start = mul(float4(ray.start, 1), object.inv_transform).xyz;
		local_ray.direction = mul(float4(ray.direction, 0), object.inv_transform).xyz;

		HitResult hit;
		if (!hit_mesh_bvh(local_ray, object_index, hit, ray_debug_info)) continue;

		if (hit.t > closest_hit.t) continue;

		closest_hit = hit;
		hit_something = true;
	}
#endif

/*
	// Compute color
	const int mesh_threshold = 64;
	const int scene_threshold = 16;
	float3 sample_color = float3(0);
	if (ray_debug_info.scene_nodes > scene_threshold ||
		ray_debug_info.mesh_aabb_tests > mesh_threshold)
	{
		sample_color = float3(1, 1, 1);
	} else {
		sample_color = float3(
				(float)ray_debug_info.scene_nodes / scene_threshold,
				0,
				(float)ray_debug_info.mesh_aabb_tests / mesh_threshold);
	}
*/

	Object object = object_buffer[closest_hit.object_index];
	float3 sample_color = mul(mul(float4(closest_hit.normal / 2 + 0.5f, 0), object.transform), transpose(object.inv_transform)).xyz;
	render_texture[coords.xy] = float4(sample_color, 1);
}
