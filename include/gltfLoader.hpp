#pragma once

#include <fastgltf/core.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>

#include "vulkan_raii.hpp"

#include "Camera.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

struct Transform {
    glm::mat4 modelMatrix;
    glm::mat4 viewProjectionMatrix;
};

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

    auto processInput(SDL_Event &e) -> void;

    [[nodiscard]]
    auto getCamera() const -> const PerspectiveCamera &;
    [[nodiscard]]
    auto getCamera() -> PerspectiveCamera &;
};

auto getGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset;

auto getSceneData(
    const fastgltf::Asset       &asset,
    const std::filesystem::path &directory) -> Scene;
