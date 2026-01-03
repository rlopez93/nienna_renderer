// src/AABB.cpp
#include "AABB.hpp"

#include <array>
#include <limits>

#include <glm/common.hpp>
#include <glm/ext/vector_float4.hpp>

#include "Asset.hpp"
#include "Geometry.hpp"
#include "SceneDrawList.hpp"

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

auto computeLocalAABB(const Primitive &primitive) -> AABB
{
    // Bounds from vertex positions only.
    if (primitive.vertices.empty()) {
        return AABB::invalid();
    }

    auto localAABB = AABB::invalid();

    for (const auto &vertex : primitive.vertices) {
        const auto &position = vertex.position;

        // Expand AABB to enclose each vertex
        localAABB.min = glm::min(localAABB.min, position);
        localAABB.max = glm::max(localAABB.max, position);
    }

    return localAABB;
}

// Compute a world-space AABB for the scene by iterating draw calls to
// account for instancing.
auto computeSceneAABB(
    const Asset         &asset,
    const SceneDrawList &sceneDrawList) -> AABB
{
    auto sceneAABB = AABB::invalid();

    for (const auto &drawItem : sceneDrawList.draws) {

        const auto &primitive =
            asset.meshes[drawItem.meshIndex].primitives[drawItem.primitiveIndex];

        const auto localAABB = computeLocalAABB(primitive);

        // Use the per-instance transform when producing world bounds.
        const auto &worldFromLocalTransform =
            sceneDrawList.nodeInstances[drawItem.nodeInstanceIndex].modelMatrix;

        const auto worldAABB =
            computeWorldAABBFromLocalAABB(localAABB, worldFromLocalTransform);

        sceneAABB.merge(worldAABB);
    }

    return sceneAABB;
}
