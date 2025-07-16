struct Primitive
{
	float3 position : POSITION;
	float3 normal : NORMAL;
    float4 tangent;
	float2 uv0 : TEXCOORD0;
    float2 uv1;
    float4 color;
}

struct FragmentPrimitive
{
	float4 position : SV_Position;
    float3 normal : NORMAL;
    float4 tangent;
	float2 uv0 : TEXCOORD0;
    float2 uv1;
    float4 color;
}

struct Transform {
    float4x4 modelMatrix;
    float4x4 ViewMatrix;
    float4x4 projectionMatrix;

    };

struct FragmentOutput
{
	float4 color : SV_TARGET;
}

ConstantBuffer<Scene> scene;

[shader("vertex")]
FragmentPrimitive vertexMain(Primitive input)
{
	FragmentPrimitive output;
	output.position = mul(scene.projection, mul(scene.view, mul(scene.model, float4(input.position, 1.0))));
    output.normal = input.normal;
    output.tangent = input.tangent;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    output.color = input.color;
	return output;
}

Sampler2D texture;

[shader("fragment")]
FragmentOutput fragmentMain(VertexOutput input) : SV_TARGET
{
	FragmentOutput output;
	output.color = mul(color, texture.Sample(input.uv0));

	return output;
}
