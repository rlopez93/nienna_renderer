#pragma once

#include <fastgltf/core.hpp>
#include <filesystem>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <vulkan/vulkan.hpp>

#include "Camera.hpp"

struct Texture {
    std::filesystem::path      name;
    std::vector<unsigned char> data;
    vk::Extent2D               extent;
};

struct Primitive {
    glm::vec3 position;
    glm::vec3 normal;
    // std::array<glm::vec2, 2> uv;
    glm::vec2 uv;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct Material {
    glm::vec4               baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    float                   metallicFactor  = 1.0f;
    float                   roughnessFactor = 1.0f;
    std::optional<uint32_t> metallicRoughnessTexture;
    std::optional<uint32_t> normalTexture;
    std::optional<uint32_t> occlusionTexture;
    std::optional<uint32_t> emissiveTexture;
    glm::vec3               emissiveFactor{0.0f, 0.0f, 0.0f};
    // TODO: alphaMode
    // TODO: alphaCutoff
    // TODO: doubleSided
};

struct Mesh {
    std::vector<Primitive>  primitives;
    std::vector<uint16_t>   indices;
    glm::vec4               color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::mat4               modelMatrix = glm::identity<glm::mat4>();
    std::optional<uint32_t> textureIndex;
};

struct Scene {
    std::vector<Mesh>              meshes;
    std::vector<Texture>           textures;
    std::vector<Material>          materials;
    std::vector<PerspectiveCamera> cameras;
    uint32_t                       cameraIndex = 0u;

    auto processInput(SDL_Event &e) -> void
    {
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            switch (e.key.scancode) {
            case SDL_SCANCODE_N:
                cameraIndex = (cameraIndex + 1) % cameras.size();
            default:
                break;
            }
        }
    }

    [[nodiscard]]
    auto getCamera() const -> const PerspectiveCamera &
    {
        return cameras[cameraIndex];
    }
    [[nodiscard]]
    auto getCamera() -> PerspectiveCamera &
    {
        return cameras[cameraIndex];
    }
};

struct Transform {
    glm::mat4 modelMatrix;
    glm::mat4 viewProjectionMatrix;
};

auto getGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset;

auto getSceneData(
    const fastgltf::Asset       &asset,
    const std::filesystem::path &directory) -> Scene;
