#include "Camera.hpp"

#include <chrono>
#include <fmt/base.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

// FIXME: Fix more than one input per frame, e.g., Left + Right, or Up + Down

[[nodiscard]]
auto PerspectiveCamera::getViewMatrix() const -> glm::mat4
{
    return glm::inverse(
        glm::translate(glm::identity<glm::mat4>(), translation)
        * glm::mat4_cast(rotation));
}

[[nodiscard]]
auto PerspectiveCamera::getProjectionMatrix() const -> glm::mat4
{
    auto projectionMatrix = [&] {
        if (zfar) {
            return glm::perspectiveRH_ZO(yfov, aspectRatio, znear, zfar.value());
        }

        else {
            return glm::infinitePerspectiveRH_ZO(yfov, aspectRatio, znear);
        }
    }();

    projectionMatrix[1][1] *= -1;

    return projectionMatrix;
}
auto PerspectiveCamera::translate(const glm::vec3 &t) -> void
{
    translation += t;
}

auto PerspectiveCamera::rotate(
    const float      angle,
    const glm::vec3 &axis) -> void
{
    rotation = glm::rotate(rotation, angle, axis);
}

auto PerspectiveCamera::getRotationAxis(
    const Rotation   rotation,
    glm::vec3       &axis,
    const glm::vec3 &currentAxis) const -> void
{
    switch (rotation) {
    case Rotation::CW:
        // axis += currentAxis;
        break;
    case Rotation::CCW:
        // axis -= currentAxis;
        break;
    case Rotation::None:
        break;
    }
}
auto PerspectiveCamera::update(const std::chrono::duration<float> &dt) -> void
{
    switch (rotateX) {
    case Rotation::CW:
        rotate(rotationSpeed * dt.count(), {1, 0, 0});
        break;
    case Rotation::CCW:
        rotate(rotationSpeed * dt.count(), {-1, 0, 0});
        break;
    case Rotation::None:
        break;
    }
    switch (rotateY) {
    case Rotation::CW:
        rotate(rotationSpeed * dt.count(), {0, 1, 0});
        break;
    case Rotation::CCW:
        rotate(rotationSpeed * dt.count(), {0, -1, 0});
        break;
    case Rotation::None:
        break;
    }

    switch (moveX) {
    case Movement::Forward:
        translation += rotation * glm::vec3(1, 0, 0) * movementSpeed * dt.count();
        break;
    case Movement::Backward:
        translation -= rotation * glm::vec3(1, 0, 0) * movementSpeed * dt.count();
        break;
    default:
        break;
    }

    switch (moveY) {
    case Movement::Forward:
        translation += rotation * glm::vec3(0, 1, 0) * movementSpeed * dt.count();
        break;
    case Movement::Backward:
        translation -= rotation * glm::vec3(0, 1, 0) * movementSpeed * dt.count();
        break;
    default:
        break;
    }

    switch (moveZ) {
    case Movement::Forward:
        translation += rotation * glm::vec3(0, 0, -1) * movementSpeed * dt.count();
        break;
    case Movement::Backward:
        translation -= rotation * glm::vec3(0, 0, -1) * movementSpeed * dt.count();
        break;
    default:
        break;
    }
}

auto PerspectiveCamera::processInput(SDL_Event &e) -> void
{

    if (e.type == SDL_EVENT_KEY_UP) {
        fmt::println("I'm up!!");
        switch (e.key.scancode) {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_S:
            moveZ = Movement::None;
            break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_D:
            moveX = Movement::None;
            break;
        case SDL_SCANCODE_F:
        case SDL_SCANCODE_R:
            moveY = Movement::None;
            break;
        case SDL_SCANCODE_Q:
        case SDL_SCANCODE_E:
            rotateX = Rotation::None;
            break;
        default:
            break;
        }
    }

    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
        fmt::println("I'm down!!");
        if (e.key.repeat) {
            fmt::println("I'm repeating!!");
        }
        switch (e.key.scancode) {
        case SDL_SCANCODE_W:
            moveZ = Movement::Forward;
            break;
        case SDL_SCANCODE_S:
            moveZ = Movement::Backward;
            break;
        case SDL_SCANCODE_A:
            moveX = Movement::Backward;
            break;
        case SDL_SCANCODE_D:
            moveX = Movement::Forward;
            break;
        case SDL_SCANCODE_R:
            moveY = Movement::Forward;
            break;
        case SDL_SCANCODE_F:
            moveY = Movement::Backward;
            break;
        case SDL_SCANCODE_Q:
            rotateX = Rotation::CCW;
            break;
        case SDL_SCANCODE_E:
            rotateX = Rotation::CW;
            break;
        default:
            break;
        }
    }
}
