struct VertexInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
}

struct VertexOutput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
}

struct Scene {
    float4x4 model;
    float4x4 view;
    float4x4 projection;

    };

struct FragmentOutput
{
	float4 color : SV_TARGET;
}

ConstantBuffer<Scene> scene;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
	VertexOutput output;
	output.position = mul(scene.projection, mul(scene.view, mul(scene.model, float4(input.position, 1.0))));
	output.uv = input.uv;
	return output;
}

Sampler2D texture;

[shader("fragment")]
FragmentOutput fragmentMain(VertexOutput input) : SV_TARGET
{
	FragmentOutput output;
	output.color = texture.Sample(input.uv);

	return output;
}
