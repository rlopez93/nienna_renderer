#include "SceneDrawList.hpp"

#include <optional>
#include <ranges>
#include <stdexcept>

#include <fmt/format.h>

#include "AABB.hpp"
#include "Asset.hpp"

namespace
{

// TODO: Stub. Will create a fallback camera from scene bounds.
auto createDefaultCamera(
    const Asset &asset,
    const AABB  &sceneAABB) -> CameraInstance
{
    (void)asset;
    (void)sceneAABB;
    return {};
}

// Build a local TRS matrix in glTF order: M = T * R * S.
auto makeLocalTransform(const Node &node) -> glm::mat4
{
    const auto t = glm::translate(glm::mat4{1.0f}, node.translation);
    const auto r = glm::mat4_cast(node.rotation);
    const auto s = glm::scale(glm::mat4{1.0f}, node.scale);

    return t * r * s;
}

// Remove the component of a along b (Gram-Schmidt "reject").
// Assumes b is normalized.
auto reject(
    const glm::vec3 &a,
    const glm::vec3 &b) -> glm::vec3
{
    return a - b * glm::dot(a, b);
}

// Normalize v if its length is >= eps.
// Returns nullopt if v is too small to normalize safely.
auto safeNormalize(
    const glm::vec3 &v,
    const float      eps) -> std::optional<glm::vec3>
{
    const auto length_v = glm::length(v);
    if (length_v < eps) {
        return std::nullopt;
    }

    return v / length_v;
}

// Extract the (possibly scaled/sheared) basis from a mat4.
// GLM is column-major: m[0], m[1], m[2] are basis columns.
auto extractBasisXY(const glm::mat4 &m) -> glm::mat3
{
    return glm::mat3{
        glm::vec3{m[0]},
        glm::vec3{m[1]},
        glm::vec3{m[2]},
    };
}

// Construct a right-handed orthonormal basis given orthonormal x,y.
auto makeRightHanded(
    const glm::vec3 &x,
    const glm::vec3 &y) -> glm::mat3
{
    const auto z = glm::cross(x, y);
    return glm::mat3{x, y, z};
}

// Orthonormalize x and y and return a right-handed rotation basis.
// This is used to "ignore scale" for camera transforms by turning the
// matrix's upper-left 3x3 into a pure rotation.
auto orthonormalizeXY(
    glm::vec3 x,
    glm::vec3 y) -> glm::mat3
{
    constexpr auto eps = 1e-8f;

    const auto xOpt = safeNormalize(x, eps);
    if (!xOpt.has_value()) {
        throw std::runtime_error{fmt::format("camera basis degenerate (x)")};
    }

    x = xOpt.value();

    y = reject(y, x);

    const auto yOpt = safeNormalize(y, eps);
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

// Extract camera translation and rotation from a world matrix while
// explicitly ignoring scale (and shear) in the rotation.
auto extractCameraTranslationRotation(const glm::mat4 &worldTransform) -> CameraTR
{
    const auto translation = glm::vec3{worldTransform[3]};

    const auto basis = extractBasisXY(worldTransform);
    const auto rotM  = orthonormalizeXY(basis[0], basis[1]);
    const auto rotQ  = glm::normalize(glm::quat_cast(rotM));

    return CameraTR{
        .translation = translation,
        .rotation    = rotQ,
    };
}

auto visitNode(
    SceneDrawList   &sceneDrawList,
    const Asset     &asset,
    std::uint32_t    nodeIndex,
    const glm::mat4 &parentWorldTransform) -> void
{
    const auto &node = asset.nodes[nodeIndex];

    const auto localTransform = makeLocalTransform(node);
    const auto worldTransform = parentWorldTransform * localTransform;

    if (node.cameraIndex.has_value()) {

        const auto cameraTR = extractCameraTranslationRotation(worldTransform);

        sceneDrawList.cameraInstances.push_back(
            CameraInstance{
                .translation = cameraTR.translation,
                .rotation    = cameraTR.rotation,
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
                .modelMatrix = worldTransform,
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

    for (const auto childIndex : node.children) {
        visitNode(sceneDrawList, asset, childIndex, worldTransform);
    }
}
} // namespace

auto buildSceneDrawList(const Asset &asset) -> SceneDrawList
{
    auto sceneDrawList = SceneDrawList{
        .sceneIndex = asset.activeScene,
    };

    if (!asset.scenes.empty()) {
        const auto &scene = asset.scenes[sceneDrawList.sceneIndex];

        for (const auto rootIndex : scene.rootNodes) {
            visitNode(sceneDrawList, asset, rootIndex, glm::mat4{1.0f});
        }
    }

    const auto sceneAABB = computeSceneAABB(asset, sceneDrawList);
    sceneDrawList.cameraInstances.push_back(createDefaultCamera(asset, sceneAABB));

    return sceneDrawList;
}
