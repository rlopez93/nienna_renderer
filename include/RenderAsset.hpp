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

struct SceneRoots {
    std::vector<std::uint32_t> rootNodeIndices;
};

struct SceneNode {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    std::optional<std::uint32_t> meshIndex;
    std::optional<std::uint32_t> cameraIndex;

    std::vector<std::uint32_t> childNodeIndices;
};

struct RenderAsset {
    std::uint32_t activeScene = 0u;

    std::vector<SceneRoots> scenes;
    std::vector<SceneNode>  nodes;

    std::vector<Mesh> meshes;

    std::vector<Material> materials;
    std::vector<Texture>  textures;

    std::vector<Camera> cameras;
};
