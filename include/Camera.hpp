#pragma once
#include <SDL3/SDL_events.h>

#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>

#include <chrono>
#include <numbers>
#include <optional>

constexpr auto pi = std::numbers::pi_v<float>;

struct PerspectiveCamera {

    [[nodiscard]]
    auto getViewMatrix() const -> glm::mat4;

    [[nodiscard]]
    auto getProjectionMatrix() const -> glm::mat4;

    auto translate(const glm::vec3 &t) -> void;
    auto rotate(
        const float      angle,
        const glm::vec3 &axis) -> void;

    auto processInput(
        SDL_Event &e,
        const std::chrono::duration<
            float,
            std::milli> &seconds) -> void;

    glm::vec3            translation;
    glm::quat            rotation;
    glm::vec3            axis;
    float                yfov;
    float                aspectRatio;
    float                znear;
    std::optional<float> zfar;

    float movementSpeed = 1.5f;      // meters
    float rotationSpeed = pi / 4.0f; // seconds

    enum class Movement { None, Forward, Backward } moveZ;
    enum class Rotation { None, CW, CCW } rotateX, rotateY;

    auto getRotationAxis(
        const Rotation   rotation,
        glm::vec3       &axis,
        const glm::vec3 &currentAxis);
};
