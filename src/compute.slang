[[vk::binding(0)]]
uniform RWTexture2D<float4> render_texture;
uniform float3 uniform_color;

[shader("compute")]
void main(int2 coords : SV_DispatchThreadID) {
	render_texture[coords.xy] = float4((float)coords.x / 320, 0, (float)coords.y / 180, 1);
}
