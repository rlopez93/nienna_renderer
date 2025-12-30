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

struct Light
{
        float3 direction;
        float intensity;
        float3 color;
        float padding;
}

struct FragmentOutput
{
	float4 color : SV_TARGET;
}

struct PushConstantBlock
{
        uint32_t transformIndex;
        int32_t textureIndex;
        uint32_t debugView;
        uint32_t padding;
        float4 baseColorFactor;
}

ConstantBuffer<Transform> transform[];
Sampler2D texture[];
ConstantBuffer<Light> light;
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
    float3 worldNormal = mul(
        (float3x3)transform[pushConstant.transformIndex].modelMatrix, input.normal);

    output.normal = normalize(worldNormal);
    output.uv = input.uv;
    output.color = input.color;
	return output;
}


[shader("fragment")]
FragmentOutput fragmentMain(FragmentPrimitive input) : SV_TARGET
{
	FragmentOutput output;


    float3 N = normalize(input.normal);

    if (dot(N, N) < 1e-5f) {
            output.color = input.color;
            return output;
    }

    // Debug view override
    if (pushConstant.debugView == 1) {
            // map normals to color
            output.color = float4(N * 0.5f + 0.5f, 1.0f);
            return output;
    }

    float3 L = normalize(-light.direction);
    float NdotL = max(dot(N, L), 0.0);

    float4 baseColor;

    baseColor = input.color * texture[pushConstant.textureIndex].Sample(input.uv) * pushConstant.baseColorFactor;

    output.color = 
        float4(
            baseColor.rgb * light.color * light.intensity * NdotL,
            baseColor.a
        );

	return output;
}
