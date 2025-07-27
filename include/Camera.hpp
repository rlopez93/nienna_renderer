#pragma once
#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>
#include <optional>

struct PerspectiveCamera {

    [[nodiscard]]
    auto getViewMatrix() const -> glm::mat4;

    [[nodiscard]]
    auto getProjectionMatrix() const -> glm::mat4;

    glm::vec3            translation;
    glm::quat            rotation;
    float                yfov;
    float                aspectRatio;
    float                znear;
    std::optional<float> zfar;
};
