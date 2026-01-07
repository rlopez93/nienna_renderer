#include "../include/ShaderInterfaceTypes.hpp"

// Vertex input must match createPipeline() attribute locations:
// 0: position (float3)
// 1: normal   (float3)
// 2: tangent  (float4)
// 3: uv0      (float2)
// 4: uv1      (float2)
// 5: color    (float4)

struct VSInput
{
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float3 normal;
    [[vk::location(2)]] float4 tangent;
    [[vk::location(3)]] float2 uv0;
    [[vk::location(4)]] float2 uv1;
    [[vk::location(5)]] float4 color;
};

struct VSOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]] float3 normal;
    [[vk::location(1)]] float2 uv0;
    [[vk::location(2)]] float2 uv1;
    [[vk::location(3)]] float4 color;
};

struct PSOutput
{
    float4 color : SV_TARGET;
};

[[vk::binding(kBindingFrameUniforms)]]
ConstantBuffer<FrameUniforms> g_frame;

[[vk::binding(kBindingNodeInstanceData)]]
StructuredBuffer<NodeInstanceData> g_nodeData;

[[vk::binding(kBindingMaterialData)]]
StructuredBuffer<MaterialData> g_materials;

[[vk::binding(kBindingImagesSrgb)]]
Texture2D<float4> g_imagesSrgb[];

[[vk::binding(kBindingImagesLinear)]]
Texture2D<float4> g_imagesLinear[];

[[vk::binding(kBindingSamplers)]]
SamplerState g_samplers[];

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_pc;

static float2 selectUv(VSOutput v, uint texCoord)
{
    return (texCoord == 0u) ? v.uv0 : v.uv1;
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;

    NodeInstanceData node =
        g_nodeData[g_pc.nodeInstanceIndex];

    float4 worldPos =
        mul(node.modelMatrix, float4(input.position, 1.0));

    output.position =
        mul(g_frame.viewProjectionMatrix, worldPos);

    float3 worldN =
        mul((float3x3)node.modelMatrix, input.normal);

    output.normal = normalize(worldN);
    output.uv0    = input.uv0;
    output.uv1    = input.uv1;
    output.color  = input.color;

    return output;
}

[shader("fragment")]
PSOutput fragmentMain(VSOutput input) : SV_TARGET
{
    PSOutput output;

    float3 N = normalize(input.normal);

    if (dot(N, N) < 1e-5)
    {
        output.color = input.color;
        return output;
    }

    if (g_pc.debugView == 1u)
    {
        output.color = float4(N * 0.5 + 0.5, 1.0);
        return output;
    }

    MaterialData mat = g_materials[g_pc.materialIndex];

    float3 L =
        normalize(-g_frame.directionalLight.direction);

    float NdotL = max(dot(N, L), 0.0);

    uint texIdx  = mat.baseColorTexture.textureIndex;
    uint sampIdx = texIdx;

    float2 uv =
        selectUv(input, mat.baseColorTexture.texCoord);

    float4 texel =
        g_imagesSrgb[texIdx].Sample(
            g_samplers[sampIdx],
            uv);

    float4 baseColor =
        input.color * texel * mat.baseColorFactor;

    float3 lit =
        baseColor.rgb
        * g_frame.directionalLight.color
        * g_frame.directionalLight.intensity
        * NdotL;

    output.color = float4(lit, baseColor.a);
    return output;
}

