#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

#include <limits>

struct Submesh;
struct RenderAsset;
struct SceneView;

struct AABB {
    glm::vec3 min{std::numeric_limits<float>::infinity()};
    glm::vec3 max{-std::numeric_limits<float>::infinity()};

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
