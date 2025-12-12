[[vk::binding(0)]]
uniform RWTexture2D<float4> render_texture;
uniform float3 uniform_color;

[shader("compute")]
void main() {
	render_texture[int2(160, 90)] = float4(uniform_color, 1);
}
