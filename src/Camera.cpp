#include "Camera.hpp"

#include <chrono>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

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
    const glm::vec3 &currentAxis)
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

auto PerspectiveCamera::processInput(
    SDL_Event &e,
    const std::chrono::duration<
        float,
        std::milli> &dt) -> void
{
    switch (rotateX) {
    case Rotation::CW:
        // rotate(rotationSpeed * dt.count() / 1000.f, {1, 0, 0});
        break;
    case Rotation::CCW:
        // rotate(rotationSpeed * dt.count() / 1000.f, {-1, 0, 0});
        break;
    case Rotation::None:
        break;
    }
    switch (rotateY) {
    case Rotation::CW:
        // rotate(rotationSpeed * dt.count() / 1000.f, {0, 1, 0});
        break;
    case Rotation::CCW:
        // rotate(rotationSpeed * dt.count() / 1000.f, {0, -1, 0});
        break;
    case Rotation::None:
        break;
    }

    switch (moveZ) {
    case Movement::Forward:
        translation.z -= movementSpeed * dt.count() / 1000.f;
        break;
    case Movement::Backward:
        translation.z += movementSpeed * dt.count() / 1000.f;
        break;
    default:
        break;
    }

    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
        switch (e.key.scancode) {
        case SDL_SCANCODE_W:
            moveZ = Movement::Forward;
            break;
        case SDL_SCANCODE_S:
            moveZ = Movement::Backward;
            break;
        case SDL_SCANCODE_A:
            rotateY = Rotation::CCW;
            break;
        case SDL_SCANCODE_D:
            rotateY = Rotation::CW;
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

    else if (e.type == SDL_EVENT_KEY_UP) {
        switch (e.key.scancode) {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_S:
            moveZ = Movement::None;
            break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_D:
            rotateY = Rotation::None;
            break;
        case SDL_SCANCODE_Q:
        case SDL_SCANCODE_E:
            rotateX = Rotation::None;
            break;
        default:
            break;
        }
    }
}
