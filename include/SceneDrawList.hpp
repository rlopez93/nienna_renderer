#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "DrawItem.hpp"

struct Asset;

struct NodeInstance {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    std::uint32_t nodeIndex = 0u;
};

struct CameraInstance {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    std::uint32_t nodeIndex   = 0u;
    std::uint32_t cameraIndex = 0u;
};

struct SceneDrawList {
    std::uint32_t               sceneIndex = 0u;
    std::vector<NodeInstance>   nodeInstances;
    std::vector<DrawItem>       draws;
    std::vector<CameraInstance> cameraInstances;
    std::uint32_t               activeCameraInstanceIndex;

    CameraInstance &activeCameraInstance()
    {
        return cameraInstances[activeCameraInstanceIndex];
    }
};

auto buildSceneDrawList(const Asset &asset) -> SceneDrawList;
