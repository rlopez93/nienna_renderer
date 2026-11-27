#include "SceneView.hpp"

#include <cstdint>
#include <numeric>
#include <optional>
#include <ranges>
#include <stdexcept>

#include <fmt/format.h>

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

// b must be normalized
auto reject(
    glm::vec3 const& a,
    glm::vec3 const& b) -> glm::vec3
{
    return a - b * glm::dot(a, b);
}

auto safeNormalize(
    glm::vec3 const& v,
    float const      eps) -> std::optional<glm::vec3>
{
    auto const length_v = glm::length(v);
    if (length_v < eps) {
        return std::nullopt;
    }

    return v / length_v;
}

auto extractBasisColumns(glm::mat4 const& m) -> glm::mat3
{
    return glm::mat3{
        glm::vec3{m[0]},
        glm::vec3{m[1]},
        glm::vec3{m[2]},
    };
}

auto makeRightHanded(
    glm::vec3 const& x,
    glm::vec3 const& y) -> glm::mat3
{
    auto const z = glm::cross(x, y);
    return glm::mat3{x, y, z};
}

auto orthonormalize(
    glm::vec3 x,
    glm::vec3 y) -> glm::mat3
{
    constexpr auto eps = 1e-8f;

    auto const xOpt = safeNormalize(x, eps);
    if (!xOpt.has_value()) {
        throw std::runtime_error{fmt::format("camera basis degenerate (x)")};
    }

    x = xOpt.value();

    y = reject(y, x);

    auto const yOpt = safeNormalize(y, eps);
    if (!yOpt.has_value()) {
        throw std::runtime_error{fmt::format("camera basis degenerate (y)")};
    }

    y = yOpt.value();

    return makeRightHanded(x, y);
}

struct CameraTR {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

auto extractCameraTranslationRotation(glm::mat4 const& worldTransform) -> CameraTR
{
    auto const translation = glm::vec3{worldTransform[3]};

    auto const basis = extractBasisColumns(worldTransform);
    auto const rotM  = orthonormalize(basis[0], basis[1]);
    auto const rotQ  = glm::normalize(glm::quat_cast(rotM));

    return CameraTR{
        .translation = translation,
        .rotation    = rotQ,
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
