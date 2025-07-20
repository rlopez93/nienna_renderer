struct Primitive
{
    float3 position;
    float3 normal;
    float2 uv;
    // float2 uv[2];
    float4 color;
}

struct FragmentPrimitive
{
    float4 position : SV_Position;
    float3 normal;
    float2 uv;
    // float2 uv[2];
    float4 color;
}

struct Transform
{
    float4x4 modelMatrix;
    float4x4 viewProjectionMatrix;
}

struct FragmentOutput
{
	float4 color : SV_TARGET;
}

ConstantBuffer<Transform> transform;

[shader("vertex")]
FragmentPrimitive vertexMain(Primitive input)
{
	FragmentPrimitive output;
	output.position =
        mul(
            transform.viewProjectionMatrix,
            mul(
                transform.modelMatrix,
                float4(input.position, 1.0)
            )
        );
    output.normal = input.normal;
    output.uv = input.uv;
    output.color = input.color;
	return output;
}

Sampler2D textures[];

[shader("fragment")]
FragmentOutput fragmentMain(FragmentPrimitive input, uniform uint32_t textureIndex) : SV_TARGET
{
	FragmentOutput output;
	// output.color = texture.Sample(input.uv[0]);
	// output.color = texture.Sample(input.uv);
	 // output.color = mul(texture.Sample(input.uv), float4(0.0, 0.0, 0.0, 0.0) );
    // output.color = input.color;
    output.color = mul(textures[textureIndex].Sample(input.uv), input.color);

	return output;
}
