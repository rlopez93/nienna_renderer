#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Camera.hpp"
#include "Geometry.hpp"
#include "Material.hpp"
#include "Texture.hpp"

struct Scene {
    std::vector<std::uint32_t> rootNodes;
};

struct Node {
    std::vector<std::uint32_t> children;

    glm::vec3 translation{0.0f};

    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    glm::vec3 scale{1.0f};

    std::optional<std::uint32_t> mesh;
    std::optional<std::uint32_t> camera;
};

struct Asset {
    std::uint32_t activeScene = 0u;

    std::vector<Scene> scenes;
    std::vector<Node>  nodes;

    std::vector<MeshAsset> meshAssets;

    std::vector<Material> materials;
    std::vector<Texture>  textures;

    std::vector<Camera> cameras;
};
