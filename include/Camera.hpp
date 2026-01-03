#pragma once

#include <numbers>
#include <optional>
#include <variant>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct PerspectiveCamera {
    float yfov = std::numbers::pi_v<float> / 3.0f;

    std::optional<float> aspectRatio;

    float                znear = 0.1f;
    std::optional<float> zfar;
};

struct OrthographicCamera {
    float xmag = 0.0f;
    float ymag = 0.0f;

    float znear = 0.0f;
    float zfar  = 0.0f;
};

using CameraModel = std::variant<PerspectiveCamera, OrthographicCamera>;

struct Camera {
    CameraModel model;

    [[nodiscard]]
    auto getViewMatrix(
        const glm::vec3 &translation,
        const glm::quat &rotation) const -> glm::mat4;

    [[nodiscard]]
    auto getProjectionMatrix(float viewportAspect) const -> glm::mat4;
};
