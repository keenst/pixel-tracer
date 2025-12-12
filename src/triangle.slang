static const float2 positions[6] = {
	float2(-1, -1),
	float2(1, -1),
	float2(1, 1),
	float2(1, 1),
	float2(-1, 1),
	float2(-1, -1)
};

static const float2 tex_coords[6] = {
	float2(0, 0),
	float2(1, 0),
	float2(1, 1),
	float2(1, 1),
	float2(0, 1),
	float2(0, 0)
};

struct VertexInfo {
	float4 position : SV_position;
	float2 texture_coordinate;
}

uniform Sampler2D test_sampler;

[shader("vertex")]
VertexInfo vertexMain(int vertex_index : SV_VertexID) {
	VertexInfo output;
	output.position = float4(positions[vertex_index], 0, 1);
	output.texture_coordinate = tex_coords[vertex_index];

	return output;
}

[shader("fragment")]
float4 fragmentMain(VertexInfo vertex_info) : SV_Target {
	return test_sampler[float2(320, 180) * vertex_info.texture_coordinate];
}
