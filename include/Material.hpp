#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

enum DefaultTextureIndex : std::uint32_t {
    kTexWhiteSrgb = 0u,
    kTexWhiteLin  = 1u,
    kTexFlatNorm  = 2u,

    kDefaultTextureCount = 3u,
};

struct UVTransform2D {
    glm::vec2 offset{0.0f};
    glm::vec2 scale{1.0f};

    float rotation = 0.0f;

    vk::Bool32 enabled = VK_FALSE;
};

struct TextureRef {
    std::uint32_t textureIndex = kTexWhiteLin;
    std::uint32_t texCoord     = 0u;

    UVTransform2D uvTransform{};
};

struct MaterialCore {
    glm::vec4 baseColorFactor{1.0f};

    float metallicFactor  = 1.0f;
    float roughnessFactor = 1.0f;

    TextureRef baseColorTexture{.textureIndex = kTexWhiteSrgb};

    TextureRef metallicRoughnessTexture{.textureIndex = kTexWhiteLin};

    TextureRef normalTexture{.textureIndex = kTexFlatNorm};

    float normalScale = 1.0f;

    TextureRef occlusionTexture{.textureIndex = kTexWhiteLin};

    float occlusionStrength = 1.0f;

    TextureRef emissiveTexture{.textureIndex = kTexWhiteSrgb};

    glm::vec3 emissiveFactor{0.0f};

    float alphaCutoff = 0.5f;

    vk::Bool32 alphaMaskEnable  = VK_FALSE;
    vk::Bool32 alphaBlendEnable = VK_FALSE;

    vk::Bool32 doubleSided = VK_FALSE;

    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
};

struct MaterialExtUnlit {
    vk::Bool32 enabled = VK_FALSE;
};

struct MaterialExtIor {
    vk::Bool32 enabled = VK_FALSE;
    float      ior     = 1.5f;
};

struct MaterialExtSpecular {
    vk::Bool32 enabled = VK_FALSE;

    float     specularFactor = 1.0f;
    glm::vec3 specularColorFactor{1.0f};

    TextureRef specularTexture{.textureIndex = kTexWhiteLin};

    TextureRef specularColorTexture{.textureIndex = kTexWhiteSrgb};
};

struct MaterialExtClearcoat {
    vk::Bool32 enabled = VK_FALSE;

    float clearcoatFactor = 0.0f;

    TextureRef clearcoatTexture{.textureIndex = kTexWhiteLin};

    float clearcoatRoughnessFactor = 0.0f;

    TextureRef clearcoatRoughnessTexture{.textureIndex = kTexWhiteLin};

    TextureRef clearcoatNormalTexture{.textureIndex = kTexFlatNorm};
};

struct Material {
    MaterialCore core{};

    MaterialExtUnlit     unlit{};
    MaterialExtIor       ior{};
    MaterialExtSpecular  specular{};
    MaterialExtClearcoat clearcoat{};
};
