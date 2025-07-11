#pragma once

#include <fastgltf/core.hpp>
#include <filesystem>
#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 texCoord{};
};

struct Scene {
    alignas(16) fastgltf::math::fmat4x4 model{};
    alignas(16) glm::mat4 view{};
    alignas(16) fastgltf::math::fmat4x4 projection{};
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
        fastgltf::math::fmat4x4,
        glm::mat4,
        fastgltf::math::fmat4x4,
        vk::SamplerCreateInfo>;
