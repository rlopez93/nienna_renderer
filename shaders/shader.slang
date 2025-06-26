struct VertexInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 uv : TEXCOORD0;
}

struct VertexOutput
{
	float4 position : SV_Position;
	float3 uv : TEXCOORD0;
}

struct FragmentOutput
{
	float4 color : SV_Target;
}

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
	VertexOutput output;
	output.position = float4(input.position, 1.0);
	output.uv = input.uv;
	return output;
}

[shader("fragment")]
FragmentOutput fragmentMain(VertexOutput input)
{
	FragmentOutput output;
	output.color = float4(0.5, 0.2, 0.1, 1.0);

	return output;
}
