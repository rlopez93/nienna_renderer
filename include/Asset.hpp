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
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    std::optional<std::uint32_t> meshIndex;
    std::optional<std::uint32_t> cameraIndex;

    std::vector<std::uint32_t> children;
};

struct Asset {
    std::uint32_t activeScene = 0u;

    std::vector<Scene> scenes;
    std::vector<Node>  nodes;

    std::vector<Mesh> meshes;

    std::vector<Material> materials;
    std::vector<Texture>  textures;

    std::vector<Camera> cameras;
};
