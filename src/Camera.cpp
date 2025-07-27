#include "Camera.hpp"

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
