#pragma once

#include "DrawItem.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float4.hpp>

#include <cstdint>
#include <vector>

struct RenderAsset;

struct NodeInstance {
    glm::mat4 modelMatrix{1.0f};

    std::uint32_t nodeIndex = 0u;
};

struct CameraInstance {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    std::uint32_t nodeIndex   = 0u;
    std::uint32_t cameraIndex = 0u;
};

struct SceneView {
    std::uint32_t             sceneIndex = 0u;
    std::vector<NodeInstance> nodeInstances;
    std::vector<DrawItem>     draws;

    std::vector<CameraInstance> cameraInstances;

    std::uint32_t activeCameraInstanceIndex = 0u;

    auto activeCameraInstance() -> CameraInstance &
    {
        return cameraInstances[activeCameraInstanceIndex];
    }
};

auto buildSceneView(const RenderAsset &asset) -> SceneView;
