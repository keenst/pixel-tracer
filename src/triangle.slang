static const float2 positions[6] = {
	float2(-1, -1),
	float2(1, -1),
	float2(1, 1),
	float2(1, 1),
	float2(-1, 1),
	float2(-1, -1)
};

static const float3 colors[6] = {
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(0, 0, 1),
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(0, 0, 1)
};

struct VertexInfo {
	float4 position : SV_position;
	float3 color;
}

[shader("vertex")]
VertexInfo vertexMain(int vertex_index : SV_VertexID) {
	VertexInfo output;
	output.color = colors[vertex_index];
	output.position = float4(positions[vertex_index], 0, 1);

	return output;
}

[shader("fragment")]
float4 fragmentMain(VertexInfo vertex_info) : SV_Target {
	return float4(vertex_info.color, 1);
}
