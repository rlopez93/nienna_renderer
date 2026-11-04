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

// Build a local TRS matrix in glTF order: M = T * R * S.
auto makeLocalTransform(SceneNode const& node) -> glm::mat4
{
    auto const t = glm::translate(glm::mat4{1.0f}, node.translation);
    auto const r = glm::mat4_cast(node.rotation);
    auto const s = glm::scale(glm::mat4{1.0f}, node.scale);

    return t * r * s;
}

// Remove the component of a along b (Gram-Schmidt "reject").
// Assumes b is normalized.
auto reject(
    glm::vec3 const& a,
    glm::vec3 const& b) -> glm::vec3
{
    return a - b * glm::dot(a, b);
}

// Normalize v if its length is >= eps.
// Returns nullopt if v is too small to normalize safely.
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

// Extract the (possibly scaled/sheared) basis from a mat4.
// GLM is column-major: m[0], m[1], m[2] are basis columns.
auto extractBasisColumns(glm::mat4 const& m) -> glm::mat3
{
    return glm::mat3{
        glm::vec3{m[0]},
        glm::vec3{m[1]},
        glm::vec3{m[2]},
    };
}

// Construct a right-handed orthonormal basis given orthonormal x,y.
auto makeRightHanded(
    glm::vec3 const& x,
    glm::vec3 const& y) -> glm::mat3
{
    auto const z = glm::cross(x, y);
    return glm::mat3{x, y, z};
}

// Orthonormalize x and y and return a right-handed rotation basis.
// This is used to "ignore scale" for camera transforms by turning the
// matrix's upper-left 3x3 into a pure rotation.
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

// Extract camera translation and rotation from a world matrix while
// explicitly ignoring scale (and shear) in the rotation.
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

        // create a nodeInstanceIndex
        auto const nodeInstanceIndex =
            static_cast<std::uint32_t>(sceneView.nodeInstances.size());

        // create a NodeInstance
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

    auto meshOffsets = std::vector<std::uint32_t>(asset.meshes.size() + 1, 0u);
    // each mesh's offset in our geometry buffers (vertex/index buffers) can be found by
    // generating prefix sums of the primitive counts in each mesh, using
    // std::transform_inclusive_scan()
    // meshOffsets[m] is the geometry index for the first primitive in mesh m
    (void)std::transform_inclusive_scan(
        asset.meshes.begin(),
        asset.meshes.end(),
        meshOffsets.begin() + 1, // we start our inclusive_scan at meshOffsets[1] so
                                 // that meshOffsets.front() == 0
        std::plus<>{},
        [](Mesh const& mesh) {
            return static_cast<std::uint32_t>(mesh.submeshes.size());
        });

    // we use the mesh offsets and the primitive index to get the final geometry index
    // for every draw call
    for (auto& draw : sceneView.draws) {
        draw.geometryIndex = meshOffsets[draw.meshIndex] + draw.submeshIndex;
    }

    return sceneView;
}
