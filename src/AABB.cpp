#include "AABB.hpp"

#include <initializer_list>

#include <glm/common.hpp>
#include <glm/ext/vector_float4.hpp>

#include "Geometry.hpp"
#include "RenderAsset.hpp"
#include "SceneView.hpp"

auto AABB::isValid() const -> bool
{
    return min.x <= max.x && min.y <= max.y && min.z <= max.z;
}

auto AABB::merge(AABB const& other) -> void
{
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

auto computeWorldAABBFromLocalAABB(
    AABB const&      localAABB,
    glm::mat4 const& worldFromLocalTransform) -> AABB
{
    if (!localAABB.isValid()) {
        return AABB{};
    }

    auto worldAABB = AABB{};

    for (auto const x : {localAABB.min.x, localAABB.max.x}) {
        for (auto const y : {localAABB.min.y, localAABB.max.y}) {
            for (auto const z : {localAABB.min.z, localAABB.max.z}) {
                auto const worldCorner =
                    glm::vec3{worldFromLocalTransform * glm::vec4{x, y, z, 1.0f}};

                worldAABB.min = glm::min(worldAABB.min, worldCorner);
                worldAABB.max = glm::max(worldAABB.max, worldCorner);
            }
        }
    }

    return worldAABB;
}

auto computeLocalAABB(Submesh const& submesh) -> AABB
{
    auto localAABB = AABB{};

    for (auto const& vertex : submesh.vertices) {
        localAABB.min = glm::min(localAABB.min, vertex.position);
        localAABB.max = glm::max(localAABB.max, vertex.position);
    }

    return localAABB;
}

auto computeSceneAABB(
    RenderAsset const& asset,
    SceneView const&   sceneView) -> AABB
{
    auto sceneAABB = AABB{};

    for (auto const& draw : sceneView.draws) {
        auto const& submesh = asset.meshes[draw.meshIndex].submeshes[draw.submeshIndex];
        auto const& modelMatrix =
            sceneView.nodeInstances[draw.nodeInstanceIndex].modelMatrix;

        auto const localAABB = computeLocalAABB(submesh);

        sceneAABB.merge(computeWorldAABBFromLocalAABB(localAABB, modelMatrix));
    }

    return sceneAABB;
}
