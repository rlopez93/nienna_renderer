#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

// clang-format off

// This header is shared between C++ and Slang.
//
// - In C++, NIENNA_ALIGN / NIENNA_CONST / NIENNA_INIT expand to alignas, inline constexpr,
//   and brace-initializers so default values match CPU behavior.
// - In Slang, NIENNA_ALIGN and NIENNA_INIT expand to nothing and NIENNA_CONST becomes static const,
//   because shader-side data is assumed to be initialized externally.

#if defined(__SLANG__)

typealias u32  = uint;
typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;
typealias mat4 = float4x4;

#define NIENNA_ALIGN(N)
#define NIENNA_CONST static const
#define NIENNA_INIT(...)

#elif defined(__cplusplus)

#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

using u32  = std::uint32_t;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;

#define NIENNA_ALIGN(N) alignas(N)
#define NIENNA_CONST inline constexpr
#define NIENNA_INIT(...) {__VA_ARGS__}

#else
#error SharedTypes.h requires C++ or Slang.
#endif

// Descriptor bindings
NIENNA_CONST u32 kBindingFrameUniforms = 0u;
NIENNA_CONST u32 kBindingNodeInstanceData    = 1u;
NIENNA_CONST u32 kBindingMaterialData  = 2u;
NIENNA_CONST u32 kBindingImagesSrgb    = 3u;
NIENNA_CONST u32 kBindingImagesLinear  = 4u;
NIENNA_CONST u32 kBindingSamplers      = 5u;

struct NIENNA_ALIGN(16) DirectionalLight {
    vec3  direction NIENNA_INIT(0.0f, -1.0f, -1.0f);
    float intensity NIENNA_INIT(1.0f);
    vec3  color     NIENNA_INIT(1.0f, 1.0f, 1.0f);
    float _pad0     NIENNA_INIT(0.0f);
};

struct NIENNA_ALIGN(16) PointLight {
    vec3  position  NIENNA_INIT(0.0f, 2.0f, 0.0f);
    float intensity NIENNA_INIT(1.0f);
    vec3  color     NIENNA_INIT(1.0f, 1.0f, 1.0f);
    float _pad0     NIENNA_INIT(0.0f);
};

struct NIENNA_ALIGN(16) FrameUniforms {
    mat4             viewProjectionMatrix NIENNA_INIT(1.0f);
    DirectionalLight directionalLight     NIENNA_INIT();
    PointLight       pointLight           NIENNA_INIT();
};

struct NIENNA_ALIGN(16) NodeInstanceData {
    mat4 modelMatrix NIENNA_INIT(1.0f);
};

struct PushConstants {
    u32 nodeInstanceIndex   NIENNA_INIT(0u);
    u32 materialIndex       NIENNA_INIT(0u);
    u32 debugView           NIENNA_INIT(0u);
    u32 _pad0               NIENNA_INIT(0u);
};

struct NIENNA_ALIGN(16) TextureTransform2DData {
    vec2  offset   NIENNA_INIT(0.0f, 0.0f);
    vec2  scale    NIENNA_INIT(1.0f, 1.0f);
    float rotation NIENNA_INIT(0.0f);
    u32   enabled  NIENNA_INIT(0u);
    float _pad0    NIENNA_INIT(0.0f);
    float _pad1    NIENNA_INIT(0.0f);
};

struct NIENNA_ALIGN(16) TextureRefData {
    u32 textureIndex NIENNA_INIT(0u);
    u32 texCoord     NIENNA_INIT(0u);
    u32 _pad0        NIENNA_INIT(0u);
    u32 _pad1        NIENNA_INIT(0u);

    TextureTransform2DData transform NIENNA_INIT();
};

NIENNA_CONST u32 kMaterialFlagAlphaMask   = 1u << 0u;
NIENNA_CONST u32 kMaterialFlagAlphaBlend  = 1u << 1u;
NIENNA_CONST u32 kMaterialFlagDoubleSided = 1u << 2u;
NIENNA_CONST u32 kMaterialFlagUnlit       = 1u << 3u;
NIENNA_CONST u32 kMaterialFlagIor         = 1u << 4u;
NIENNA_CONST u32 kMaterialFlagSpecular    = 1u << 5u;
NIENNA_CONST u32 kMaterialFlagClearcoat   = 1u << 6u;

struct NIENNA_ALIGN(16) MaterialData {
    vec4 baseColorFactor NIENNA_INIT(0.0f, 0.0f, 0.0f, 0.0f);

    vec3  emissiveFactor    NIENNA_INIT(0.0f, 0.0f, 0.0f); 
    float metallicFactor    NIENNA_INIT(0.0f);

    float roughnessFactor   NIENNA_INIT(0.0f);
    float normalScale       NIENNA_INIT(0.0f);
    float occlusionStrength NIENNA_INIT(0.0f);
    float alphaCutoff       NIENNA_INIT(0.0f);

    u32   flags           NIENNA_INIT(0u);
    float ior             NIENNA_INIT(0.0f);
    float specularFactor  NIENNA_INIT(0.0f);
    float clearcoatFactor NIENNA_INIT(0.0f);

    float clearcoatRoughnessFactor NIENNA_INIT(0.0f);
    float _pad0                    NIENNA_INIT(0.0f);
    float _pad1                    NIENNA_INIT(0.0f);
    float _pad2                    NIENNA_INIT(0.0f);

    vec3  specularColorFactor NIENNA_INIT( 0.0f, 0.0f, 0.0f);
    float _pad3               NIENNA_INIT(0.0f);

    TextureRefData baseColorTexture          NIENNA_INIT();
    TextureRefData metallicRoughnessTexture  NIENNA_INIT();
    TextureRefData normalTexture             NIENNA_INIT();
    TextureRefData occlusionTexture          NIENNA_INIT();
    TextureRefData emissiveTexture           NIENNA_INIT();
    TextureRefData specularTexture           NIENNA_INIT();
    TextureRefData specularColorTexture      NIENNA_INIT();
    TextureRefData clearcoatTexture          NIENNA_INIT();
    TextureRefData clearcoatRoughnessTexture NIENNA_INIT();
    TextureRefData clearcoatNormalTexture    NIENNA_INIT();
};

#if defined(__cplusplus)
static_assert(sizeof(glm::vec3) == 12u);
static_assert(sizeof(glm::mat4) == 64u);

static_assert(alignof(DirectionalLight) == 16u);
static_assert(sizeof(DirectionalLight) == 32u);

static_assert(alignof(PointLight) == 16u);
static_assert(sizeof(PointLight) == 32u);

static_assert(alignof(FrameUniforms) == 16u);
static_assert(sizeof(FrameUniforms) == 128u);

static_assert(alignof(NodeInstanceData) == 16u);
static_assert(sizeof(NodeInstanceData) == 64u);

static_assert(sizeof(PushConstants) == 16u);

static_assert(alignof(TextureTransform2DData) == 16u);
static_assert(sizeof(TextureTransform2DData) == 32u);

static_assert(alignof(TextureRefData) == 16u);
static_assert(sizeof(TextureRefData) == 48u);

static_assert(alignof(MaterialData) == 16u);
#endif

#undef NIENNA_ALIGN
#undef NIENNA_CONST
#undef NIENNA_INIT

// clang-format on
#endif // SHARED_TYPES_H
