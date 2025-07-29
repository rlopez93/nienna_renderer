#pragma once
#include <SDL3/SDL_events.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <glm/ext/quaternion_float.hpp>

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

    auto update(const std::chrono::duration<float> &dt) -> void;

    auto processInput(SDL_Event &e) -> void;

    glm::vec3            translation{0.0f, 0.0f, 1.0f};
    glm::quat            rotation;
    float                yfov        = 0.66f;
    float                aspectRatio = 1.5f;
    float                znear       = 1.0f;
    std::optional<float> zfar        = 1000.0f;

    float movementSpeed = 1.5f;      // meters per second
    float rotationSpeed = pi / 4.0f; // seconds

    enum class Movement { None, Forward, Backward } moveX, moveY, moveZ;
    enum class Rotation { None, CW, CCW } rotateX, rotateY;

    auto getRotationAxis(
        const Rotation   rotation,
        glm::vec3       &axis,
        const glm::vec3 &currentAxis);
};
