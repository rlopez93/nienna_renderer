#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

struct Submesh;
struct RenderAsset;
struct SceneView;

struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    static auto invalid() -> AABB;

    auto isValid() const -> bool;

    auto merge(AABB const& other) -> void;
};

auto computeWorldAABBFromLocalAABB(
    AABB const&      localAABB,
    glm::mat4 const& worldFromLocalTransform) -> AABB;

auto computeLocalAABB(Submesh const& submesh) -> AABB;

auto computeSceneAABB(
    RenderAsset const& asset,
    SceneView const&   sceneView) -> AABB;
