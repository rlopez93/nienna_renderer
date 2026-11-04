#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

struct Submesh;
struct RenderAsset;
struct SceneView;

// Axis-aligned bounding box in 3D.
struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    // Sentinel for "no bounds yet".
    static auto invalid() -> AABB;

    auto isValid() const -> bool;

    // Union other into *this.
    auto merge(AABB const& other) -> void;
};

// Compute a world-space AABB that encloses localAABB after applying
// worldFromLocalTransform.
auto computeWorldAABBFromLocalAABB(
    AABB const&      localAABB,
    glm::mat4 const& worldFromLocalTransform) -> AABB;

// Compute a primitive's local-space AABB from vertex positions.
auto computeLocalAABB(Submesh const& submesh) -> AABB;

// Compute the active scene's world-space AABB from draw calls.
auto computeSceneAABB(
    RenderAsset const& asset,
    SceneView const&   sceneView) -> AABB;
