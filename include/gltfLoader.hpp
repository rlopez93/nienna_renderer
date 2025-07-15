#pragma once

#include <fastgltf/core.hpp>
#include <filesystem>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 texCoord{};
};

struct SceneInfo {
    alignas(16) glm::mat4 model{};
    alignas(16) glm::mat4 view{};
    alignas(16) glm::mat4 projection{};
};

auto getGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset;

auto getGltfAssetData(
    fastgltf::Asset             &asset,
    const std::filesystem::path &path)
    -> std::tuple<
        std::vector<uint16_t>,
        std::vector<Vertex>,
        std::vector<unsigned char>,
        vk::Extent2D,
        glm::mat4,
        glm::mat4,
        glm::mat4,
        vk::SamplerCreateInfo>;
