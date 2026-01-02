#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

enum DefaultTextureIndex : std::uint32_t {
    kTexWhiteSrgb   = 0u,
    kTexWhiteLinear = 1u,
    kTexFlatNormals = 2u,

    kDefaultTextureCount = 3u,
};

struct UVTransform2D {
    glm::vec2 offset{0.0f};
    glm::vec2 scale{1.0f};

    float rotation = 0.0f;

    vk::Bool32 enabled = VK_FALSE;
};

struct TextureRef {
    std::uint32_t textureIndex = kTexWhiteLinear;
    std::uint32_t texCoord     = 0u;

    UVTransform2D uvTransform{};
};

struct MaterialCore {
    glm::vec4 baseColorFactor{1.0f};

    float metallicFactor  = 1.0f;
    float roughnessFactor = 1.0f;

    TextureRef baseColorTexture{.textureIndex = kTexWhiteSrgb};

    TextureRef metallicRoughnessTexture{.textureIndex = kTexWhiteLinear};

    TextureRef normalTexture{.textureIndex = kTexFlatNormals};

    float normalScale = 1.0f;

    TextureRef occlusionTexture{.textureIndex = kTexWhiteLinear};

    float occlusionStrength = 1.0f;

    TextureRef emissiveTexture{.textureIndex = kTexWhiteSrgb};

    glm::vec3 emissiveFactor{0.0f};

    float alphaCutoff = 0.5f;

    vk::Bool32 alphaMaskEnable  = vk::False;
    vk::Bool32 alphaBlendEnable = vk::False;

    vk::Bool32 doubleSided = vk::False;

    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
};

struct MaterialExtUnlit {
    vk::Bool32 enabled = vk::False;
};

struct MaterialExtIor {
    vk::Bool32 enabled = vk::False;
    float      ior     = 1.5f;
};

struct MaterialExtSpecular {
    vk::Bool32 enabled = vk::False;

    float     specularFactor = 1.0f;
    glm::vec3 specularColorFactor{1.0f};

    TextureRef specularTexture{.textureIndex = kTexWhiteLinear};

    TextureRef specularColorTexture{.textureIndex = kTexWhiteSrgb};
};

struct MaterialExtClearcoat {
    vk::Bool32 enabled = VK_FALSE;

    float clearcoatFactor = 0.0f;

    TextureRef clearcoatTexture{.textureIndex = kTexWhiteLinear};

    float clearcoatRoughnessFactor = 0.0f;

    TextureRef clearcoatRoughnessTexture{.textureIndex = kTexWhiteLinear};

    TextureRef clearcoatNormalTexture{.textureIndex = kTexFlatNormals};
};

struct Material {
    MaterialCore core{};

    MaterialExtUnlit     unlit{};
    MaterialExtIor       ior{};
    MaterialExtSpecular  specular{};
    MaterialExtClearcoat clearcoat{};
};
