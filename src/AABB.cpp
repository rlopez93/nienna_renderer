#include "AABB.hpp"

#include <array>
#include <limits>

#include <glm/common.hpp>
#include <glm/ext/vector_float4.hpp>

#include "Geometry.hpp"
#include "RenderAsset.hpp"
#include "SceneView.hpp"

auto AABB::invalid() -> AABB
{
    auto const inf = std::numeric_limits<float>::infinity();

    return AABB{
        .min = glm::vec3{inf},
        .max = glm::vec3{-inf},
    };
}

auto AABB::isValid() const -> bool
{
    return min.x <= max.x && min.y <= max.y && min.z <= max.z;
}

auto AABB::merge(const AABB& other) -> void
{
    if (!other.isValid()) {
        return;
    }

    if (!isValid()) {
        *this = other;
        return;
    }

    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

auto computeWorldAABBFromLocalAABB(
    const AABB&      localAABB,
    glm::mat4 const& worldFromLocalTransform) -> AABB
{
    if (!localAABB.isValid()) {
        return AABB::invalid();
    }

    auto const cornerLocalPositions = std::array{
        glm::vec3{
            localAABB.min.x,
            localAABB.min.y,
            localAABB.min.z,
        },
        glm::vec3{
            localAABB.min.x,
            localAABB.min.y,
            localAABB.max.z,
        },
        glm::vec3{
            localAABB.min.x,
            localAABB.max.y,
            localAABB.min.z,
        },
        glm::vec3{
            localAABB.min.x,
            localAABB.max.y,
            localAABB.max.z,
        },
        glm::vec3{
            localAABB.max.x,
            localAABB.min.y,
            localAABB.min.z,
        },
        glm::vec3{
            localAABB.max.x,
            localAABB.min.y,
            localAABB.max.z,
        },
        glm::vec3{
            localAABB.max.x,
            localAABB.max.y,
            localAABB.min.z,
        },
        glm::vec3{
            localAABB.max.x,
            localAABB.max.y,
            localAABB.max.z,
        },
    };

    auto worldAABB = AABB::invalid();

    for (auto const& cornerLocalPosition : cornerLocalPositions) {

        auto const cornerLocalPositionHomogeneous =
            glm::vec4{cornerLocalPosition, 1.0f};

        auto const cornerWorldPositionHomogeneous =
            worldFromLocalTransform * cornerLocalPositionHomogeneous;

        auto const cornerWorldPosition = glm::vec3{
            cornerWorldPositionHomogeneous.x,
            cornerWorldPositionHomogeneous.y,
            cornerWorldPositionHomogeneous.z,
        };

        worldAABB.min = glm::min(worldAABB.min, cornerWorldPosition);
        worldAABB.max = glm::max(worldAABB.max, cornerWorldPosition);
    }

    return worldAABB;
}

auto computeLocalAABB(Submesh const& submesh) -> AABB
{
    if (submesh.vertices.empty()) {
        return AABB::invalid();
    }

    auto localAABB = AABB::invalid();

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
    auto sceneAABB = AABB::invalid();

    for (auto const& drawItem : sceneView.draws) {

        auto const& submesh =
            asset.meshes[drawItem.meshIndex].submeshes[drawItem.submeshIndex];

        auto const localAABB = computeLocalAABB(submesh);

        auto const& worldFromLocalTransform =
            sceneView.nodeInstances[drawItem.nodeInstanceIndex].modelMatrix;

        auto const worldAABB =
            computeWorldAABBFromLocalAABB(localAABB, worldFromLocalTransform);

        sceneAABB.merge(worldAABB);
    }

    return sceneAABB;
}
