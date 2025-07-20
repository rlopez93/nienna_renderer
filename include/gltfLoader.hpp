#pragma once

#include <fastgltf/core.hpp>
#include <filesystem>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <vulkan/vulkan.hpp>

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

struct Mesh {
    std::vector<Primitive>  primitives;
    std::vector<uint16_t>   indices;
    glm::vec4               color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::mat4               modelMatrix = glm::identity<glm::mat4>();
    std::optional<uint32_t> textureIndex;
};

struct Scene {
    Scene()
    {
        auto viewMatrix = glm::lookAt(
            glm::vec3{0.0f, 0.0f, 3.0f},
            glm::vec3{0.0f, 0.0f, -1.0f},
            glm::vec3{0.0f, 1.0f, 0.0f});
        auto projectionMatrix = glm::perspectiveRH_ZO(0.66f, 1.5f, 1.0f, 1000.0f);
        projectionMatrix[1][1] *= -1;

        viewProjectionMatrix = projectionMatrix * viewMatrix;
    }

    std::vector<Mesh>    meshes;
    std::vector<Texture> textures;
    glm::mat4            viewProjectionMatrix;
};

struct Transform {
    glm::mat4 modelMatrix;
    glm::mat4 viewProjectionMatrix;
};

auto getGltfAsset(const std::filesystem::path &gltfPath) -> fastgltf::Asset;

auto getSceneData(
    fastgltf::Asset             &asset,
    const std::filesystem::path &directory) -> Scene;
