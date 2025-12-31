#pragma once

#include "Camera.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

#include <SDL3/SDL_events.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

#include <filesystem>

struct Light {
    glm::vec3 direction = {0.0f, -1.0f, -1.0f};
    float     intensity = 1.0f;
    glm::vec3 color     = {1.0f, 1.0f, 1.0f};
    float     padding;
};

struct PointLight {
    glm::vec3 position{0.0f, 2.0f, 0.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float     intensity = 1.0f;

    float moveSpeed = 5.0f;
};

struct Transform {
    glm::mat4 modelMatrix;
    glm::mat4 viewProjectionMatrix;
};

struct Texture {
    std::filesystem::path      name = "";
    std::vector<unsigned char> data = {
        255, 255, 255, 255}; // Default (1.0, 1.0, 1.0, 1.0)
    vk::Extent2D extent = vk::Extent2D{1u, 1u};
};

struct Primitive {
    glm::vec3 position;
    glm::vec3 normal{0, 0, 0};
    // std::array<glm::vec2, 2> uv;
    glm::vec2 uv{0, 0};
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
    std::vector<Primitive> primitives;
    std::vector<uint16_t>  indices;
    glm::vec4              color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::mat4              modelMatrix  = glm::identity<glm::mat4>();
    uint32_t               textureIndex = 0u;
};

struct Scene {
    std::vector<Mesh>              meshes;
    std::vector<Texture>           textures;
    std::vector<Material>          materials;
    std::vector<PerspectiveCamera> cameras;
    uint32_t                       cameraIndex = 0u;
    PointLight                     pointLight;

    auto processInput(SDL_Event &e) -> void;
    auto update(const std::chrono::duration<float> dt) -> void;

    [[nodiscard]]
    auto getCamera() const -> const PerspectiveCamera &;
    [[nodiscard]]
    auto getCamera() -> PerspectiveCamera &;
};
