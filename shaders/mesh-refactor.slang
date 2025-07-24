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

struct PushConstantBlock
{
        uint32_t transformIndex;
        int32_t textureIndex;
        float4 baseColorFactor;
}

ConstantBuffer<Transform> transform[];
Sampler2D texture[];
[[vk::push_constant]] ConstantBuffer<PushConstantBlock> pushConstant;


[shader("vertex")]
FragmentPrimitive vertexMain(Primitive input)
{
	FragmentPrimitive output;
	output.position =
        mul(
            transform[pushConstant.transformIndex].viewProjectionMatrix,
            mul(
                transform[pushConstant.transformIndex].modelMatrix,
                float4(input.position, 1.0)
            )
        );
    output.normal = input.normal;
    output.uv = input.uv;
    output.color = input.color * pushConstant.baseColorFactor;
	return output;
}



[shader("fragment")]
FragmentOutput fragmentMain(FragmentPrimitive input) : SV_TARGET
{
	FragmentOutput output;
    if (pushConstant.textureIndex < 0) {
        output.color = input.color;
    } else {
        output.color = texture[pushConstant.textureIndex].Sample(input.uv) * input.color;
    }

	return output;
}
