#include "SceneDrawList.hpp"

#include <cstdint>
#include <ranges>

#include "Asset.hpp"

namespace
{

struct TRS {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

auto composeTRS(
    const TRS &parent,
    const TRS &local) -> TRS
{
    return TRS{
        .translation =
            parent.translation + (parent.rotation * (parent.scale * local.translation)),
        .rotation = parent.rotation * local.rotation,
        .scale    = parent.scale * local.scale,
    };
}

auto visitNode(
    SceneDrawList &sceneDrawList,
    const Asset   &asset,
    std::uint32_t  nodeIndex,
    const TRS     &parentWorld) -> void
{
    const auto &node = asset.nodes[nodeIndex];

    const auto local = TRS{
        .translation = node.translation,
        .rotation    = node.rotation,
        .scale       = node.scale,
    };

    const auto world = composeTRS(parentWorld, local);

    if (node.cameraIndex.has_value()) {

        sceneDrawList.cameraInstances.push_back(
            CameraInstance{
                .translation = world.translation,
                .rotation    = world.rotation,
                .nodeIndex   = nodeIndex,
                .cameraIndex = static_cast<std::uint32_t>(*node.cameraIndex),
            });
    }

    if (node.meshIndex.has_value()) {
        const auto meshIndex = node.meshIndex.value();
        const auto nodeInstanceIndex =
            static_cast<std::uint32_t>(sceneDrawList.nodeInstances.size());

        sceneDrawList.nodeInstances.push_back(
            NodeInstance{
                .translation = world.translation,
                .rotation    = world.rotation,
                .scale       = world.scale,
                .nodeIndex   = nodeIndex,
            });

        const auto &mesh = asset.meshes[meshIndex];
        for (const auto &[primitiveIndex, primitive] :
             std::views::enumerate(mesh.primitives)) {

            sceneDrawList.draws.push_back(
                DrawItem{
                    .indexCount = static_cast<std::uint32_t>(primitive.indices.size()),
                    .firstIndex = 0u,
                    .vertexOffset      = 0,
                    .meshIndex         = meshIndex,
                    .primitiveIndex    = static_cast<std::uint32_t>(primitiveIndex),
                    .geometryIndex     = 0u,
                    .nodeInstanceIndex = nodeInstanceIndex,
                    .materialIndex     = primitive.materialIndex,
                });
        }
    }

    for (auto childIndex : node.children) {
        visitNode(sceneDrawList, asset, childIndex, world);
    }
}

} // namespace

auto buildSceneDrawList(const Asset &asset) -> SceneDrawList
{
    SceneDrawList runtimeScene{
        .sceneIndex = asset.activeScene,
    };

    if (asset.scenes.empty()) {
        return runtimeScene;
    }

    const auto &scene = asset.scenes[runtimeScene.sceneIndex];

    const auto identity = TRS{};

    for (auto rootIndex : scene.rootNodes) {
        visitNode(runtimeScene, asset, rootIndex, identity);
    }

    // TODO (camera B):
    // If out.activeCamera is empty, use a fallback camera.

    return runtimeScene;
}
