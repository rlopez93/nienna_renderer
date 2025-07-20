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

struct Index
{
        uint32_t transformIndex;
        int32_t textureIndex;
}

ConstantBuffer<Transform> transform[];
Sampler2D texture[];
[[vk::push_constant]] ConstantBuffer<Index> index;


[shader("vertex")]
FragmentPrimitive vertexMain(Primitive input)
{
	FragmentPrimitive output;
	output.position =
        mul(
            transform[index.transformIndex].viewProjectionMatrix,
            mul(
                transform[index.transformIndex].modelMatrix,
                float4(input.position, 1.0)
            )
        );
    output.normal = input.normal;
    output.uv = input.uv;
    output.color = input.color;
	return output;
}



[shader("fragment")]
FragmentOutput fragmentMain(FragmentPrimitive input) : SV_TARGET
{
	FragmentOutput output;
    if (index.textureIndex < 0) {
        output.color = input.color;
    } else {
        output.color = texture[index.textureIndex].Sample(input.uv) * input.color;
    }

	return output;
}
