#pragma once

#include <cstdint>

#include <glm/glm.hpp>

static_assert(sizeof(glm::vec3) == 12u);
static_assert(sizeof(glm::mat4) == 64u);

inline constexpr std::uint32_t kBindingFrameUniforms = 0u;
inline constexpr std::uint32_t kBindingObjectData    = 1u;
inline constexpr std::uint32_t kBindingMaterialData  = 2u;
inline constexpr std::uint32_t kBindingImagesSrgb    = 3u;
inline constexpr std::uint32_t kBindingImagesLinear  = 4u;
inline constexpr std::uint32_t kBindingSamplers      = 5u;

struct alignas(16) DirectionalLight {
    glm::vec3 direction{0.0f, -1.0f, -1.0f};
    float     intensity = 1.0f;
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float     _pad0 = 0.0f;
};

static_assert(alignof(DirectionalLight) == 16u);
static_assert(sizeof(DirectionalLight) == 32u);

struct alignas(16) PointLight {
    glm::vec3 position{0.0f, 2.0f, 0.0f};
    float     intensity = 1.0f;
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float     _pad0 = 0.0f;
};

static_assert(alignof(PointLight) == 16u);
static_assert(sizeof(PointLight) == 32u);

struct alignas(16) FrameUniforms {
    glm::mat4 viewProjectionMatrix{1.0f};

    DirectionalLight directionalLight{};

    PointLight pointLight{};
};

static_assert(alignof(FrameUniforms) == 16u);
static_assert(sizeof(FrameUniforms) == 128u);

struct alignas(16) ObjectData {
    glm::mat4 modelMatrix{1.0f};
};

static_assert(alignof(ObjectData) == 16u);
static_assert(sizeof(ObjectData) == 64u);

struct PushConstants {
    std::uint32_t objectIndex   = 0u;
    std::uint32_t materialIndex = 0u;
    std::uint32_t debugView     = 0u;
    std::uint32_t _pad0         = 0u;
};

static_assert(sizeof(PushConstants) == 16u);

struct alignas(16) TextureTransform2DData {
    glm::vec2     offset{};
    glm::vec2     scale{};
    float         rotation = 0.0f;
    std::uint32_t enabled  = 0u;
    float         _pad0    = 0.0f;
    float         _pad1    = 0.0f;
};

static_assert(alignof(TextureTransform2DData) == 16u);
static_assert(sizeof(TextureTransform2DData) == 32u);

struct alignas(16) TextureRefData {
    std::uint32_t textureIndex = 0u;
    std::uint32_t texCoord     = 0u;
    std::uint32_t _pad0        = 0u;
    std::uint32_t _pad1        = 0u;

    TextureTransform2DData transform{};
};

static_assert(alignof(TextureRefData) == 16u);
static_assert(sizeof(TextureRefData) == 48u);

inline constexpr std::uint32_t kMaterialFlagAlphaMask   = 1u << 0u;
inline constexpr std::uint32_t kMaterialFlagAlphaBlend  = 1u << 1u;
inline constexpr std::uint32_t kMaterialFlagDoubleSided = 1u << 2u;
inline constexpr std::uint32_t kMaterialFlagUnlit       = 1u << 3u;
inline constexpr std::uint32_t kMaterialFlagIor         = 1u << 4u;
inline constexpr std::uint32_t kMaterialFlagSpecular    = 1u << 5u;
inline constexpr std::uint32_t kMaterialFlagClearcoat   = 1u << 6u;

struct alignas(16) MaterialData {
    glm::vec4 baseColorFactor{};

    glm::vec3 emissiveFactor{};
    float     metallicFactor = 0.0f;

    float roughnessFactor   = 0.0f;
    float normalScale       = 0.0f;
    float occlusionStrength = 0.0f;
    float alphaCutoff       = 0.0f;

    std::uint32_t flags           = 0u;
    float         ior             = 0.0f;
    float         specularFactor  = 0.0f;
    float         clearcoatFactor = 0.0f;

    float clearcoatRoughnessFactor = 0.0f;
    float _pad0                    = 0.0f;
    float _pad1                    = 0.0f;
    float _pad2                    = 0.0f;

    glm::vec3 specularColorFactor{};
    float     _pad3 = 0.0f;

    TextureRefData baseColorTexture{};
    TextureRefData metallicRoughnessTexture{};
    TextureRefData normalTexture{};
    TextureRefData occlusionTexture{};
    TextureRefData emissiveTexture{};

    TextureRefData specularTexture{};
    TextureRefData specularColorTexture{};

    TextureRefData clearcoatTexture{};
    TextureRefData clearcoatRoughnessTexture{};
    TextureRefData clearcoatNormalTexture{};
};

static_assert(alignof(MaterialData) == 16u);
