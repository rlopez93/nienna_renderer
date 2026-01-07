// src/AABB.cpp
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
    // Invalid sentinel where min > max component-wise.
    const auto inf = std::numeric_limits<float>::infinity();

    return AABB{
        .min = glm::vec3{inf},
        .max = glm::vec3{-inf},
    };
}

auto AABB::isValid() const -> bool
{
    // Valid iff min <= max component-wise.
    return min.x <= max.x && min.y <= max.y && min.z <= max.z;
}

auto AABB::merge(const AABB &other) -> void
{
    if (!other.isValid()) {
        return;
    }

    if (!isValid()) {
        *this = other;
        return;
    }

    // Expand *this AABB to enclose other AABB as well
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

auto computeWorldAABBFromLocalAABB(
    const AABB      &localAABB,
    const glm::mat4 &worldFromLocalTransform) -> AABB
{
    if (!localAABB.isValid()) {
        return AABB::invalid();
    }

    // Transform local AABB via its corners, then enclose.

    const auto cornerLocalPositions = std::array{
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

    for (const auto &cornerLocalPosition : cornerLocalPositions) {

        // Promote corner position to homogeneous coordinates (append w = 1)
        const auto cornerLocalPositionHomogeneous =
            glm::vec4{cornerLocalPosition, 1.0f};

        // Transform the homogenous corner from local to world-space
        const auto cornerWorldPositionHomogeneous =
            worldFromLocalTransform * cornerLocalPositionHomogeneous;

        // Project back to 3D world-space position by dropping w
        const auto cornerWorldPosition = glm::vec3{
            cornerWorldPositionHomogeneous.x,
            cornerWorldPositionHomogeneous.y,
            cornerWorldPositionHomogeneous.z,
        };

        // Expand the world-space AABB to enclose the transformed corner
        worldAABB.min = glm::min(worldAABB.min, cornerWorldPosition);
        worldAABB.max = glm::max(worldAABB.max, cornerWorldPosition);
    }

    return worldAABB;
}

auto computeLocalAABB(const Submesh &submesh) -> AABB
{
    // Bounds from vertex positions only.
    if (submesh.vertices.empty()) {
        return AABB::invalid();
    }

    auto localAABB = AABB::invalid();

    for (const auto &vertex : submesh.vertices) {
        // Expand AABB to enclose each vertex
        localAABB.min = glm::min(localAABB.min, vertex.position);
        localAABB.max = glm::max(localAABB.max, vertex.position);
    }

    return localAABB;
}

// Compute a world-space AABB for the scene by iterating draw calls to
// account for instancing.
auto computeSceneAABB(
    const RenderAsset &asset,
    const SceneView   &sceneView) -> AABB
{
    auto sceneAABB = AABB::invalid();

    for (const auto &drawItem : sceneView.draws) {

        const auto &submesh =
            asset.meshes[drawItem.meshIndex].submeshes[drawItem.submeshIndex];

        const auto localAABB = computeLocalAABB(submesh);

        // Use the per-instance transform when producing world bounds.
        const auto &worldFromLocalTransform =
            sceneView.nodeInstances[drawItem.nodeInstanceIndex].modelMatrix;

        const auto worldAABB =
            computeWorldAABBFromLocalAABB(localAABB, worldFromLocalTransform);

        sceneAABB.merge(worldAABB);
    }

    return sceneAABB;
}
