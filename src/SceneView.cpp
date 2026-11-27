#include "SceneView.hpp"

#include <cstdint>
#include <numeric>
#include <ranges>
#include <stdexcept>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include "RenderAsset.hpp"

namespace
{

auto makeLocalTransform(SceneNode const& node) -> glm::mat4
{
    auto const t = glm::translate(glm::mat4{1.0f}, node.translation);
    auto const r = glm::mat4_cast(node.rotation);
    auto const s = glm::scale(glm::mat4{1.0f}, node.scale);

    return t * r * s;
}

struct CameraTR {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

auto extractCameraTranslationRotation(glm::mat4 const& worldTransform) -> CameraTR
{
    constexpr auto eps = 1e-8f;

    auto const column0 = glm::vec3{worldTransform[0]};
    auto const column1 = glm::vec3{worldTransform[1]};

    if (glm::length(column0) < eps) {
        throw std::runtime_error{"camera basis degenerate (x)"};
    }

    auto const x = glm::normalize(column0);

    auto const yOrthogonal = column1 - x * glm::dot(column1, x);

    if (glm::length(yOrthogonal) < eps) {
        throw std::runtime_error{"camera basis degenerate (y)"};
    }

    auto const y = glm::normalize(yOrthogonal);

    return CameraTR{
        .translation = glm::vec3{worldTransform[3]},
        .rotation = glm::normalize(glm::quat_cast(glm::mat3{x, y, glm::cross(x, y)})),
    };
}

auto visitNode(
    SceneView&         sceneView,
    RenderAsset const& asset,
    std::uint32_t      nodeIndex,
    glm::mat4 const&   parentWorldTransform) -> void
{
    auto const& node = asset.nodes[nodeIndex];

    auto const localTransform = makeLocalTransform(node);

    auto const worldTransform = parentWorldTransform * localTransform;

    if (node.cameraIndex.has_value()) {

        auto const cameraTR = extractCameraTranslationRotation(worldTransform);

        sceneView.cameraInstances.push_back(
            CameraInstance{
                .translation = cameraTR.translation,
                .rotation    = cameraTR.rotation,
                .nodeIndex   = nodeIndex,
                .cameraIndex = static_cast<std::uint32_t>(*node.cameraIndex),
            });
    }

    if (node.meshIndex.has_value()) {

        auto const meshIndex = node.meshIndex.value();

        auto const nodeInstanceIndex =
            static_cast<std::uint32_t>(sceneView.nodeInstances.size());

        sceneView.nodeInstances.push_back(
            NodeInstance{
                .modelMatrix = worldTransform,
                .nodeIndex   = nodeIndex,
            });

        for (auto const& [submeshIndex, submesh] :
             std::views::enumerate(asset.meshes[meshIndex].submeshes)) {

            sceneView.draws.push_back(
                DrawItem{
                    .indexCount    = static_cast<std::uint32_t>(submesh.indices.size()),
                    .firstIndex    = 0u,
                    .vertexOffset  = 0,
                    .meshIndex     = meshIndex,
                    .submeshIndex  = static_cast<std::uint32_t>(submeshIndex),
                    .geometryIndex = 0u,
                    .nodeInstanceIndex = nodeInstanceIndex,
                    .materialIndex     = submesh.materialIndex,
                });
        }
    }

    for (auto const childIndex : node.childNodeIndices) {
        visitNode(sceneView, asset, childIndex, worldTransform);
    }
}
} // namespace

auto buildSceneView(RenderAsset const& asset) -> SceneView
{
    auto sceneView = SceneView{
        .sceneIndex = asset.activeScene,
    };

    if (asset.scenes.empty()) {
        return sceneView;
    }

    auto const& sceneRoots = asset.scenes[sceneView.sceneIndex];

    for (auto const rootIndex : sceneRoots.rootNodeIndices) {
        visitNode(sceneView, asset, rootIndex, glm::mat4{1.0f});
    }

    // meshOffsets[m]: index of mesh m's first primitive in RenderableResources'
    // vertex/index buffers (same order as RenderableResources::create())
    auto meshOffsets = std::vector<std::uint32_t>(asset.meshes.size() + 1, 0u);
    (void)std::transform_inclusive_scan(
        asset.meshes.begin(),
        asset.meshes.end(),
        meshOffsets.begin() + 1,
        std::plus<>{},
        [](Mesh const& mesh) {
            return static_cast<std::uint32_t>(mesh.submeshes.size());
        });

    for (auto& draw : sceneView.draws) {
        draw.geometryIndex = meshOffsets[draw.meshIndex] + draw.submeshIndex;
    }

    return sceneView;
}
