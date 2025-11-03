[shader("vertex")]
float4 vertexMain(int vertex_index : SV_VertexID) : SV_Position {
	float2 position[3] = {
		float2(0, -0.5),
		float2(0.5, 0.5),
		float2(-0.5, 0.5)
	};

	return float4(position[vertex_index], 0, 0);
}

[shader("fragment")]
float4 fragmentMain(float4 sv_position : SV_Position) : SV_Target {
	return float4(1, 0, 0, 0);
}
