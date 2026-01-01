#include "Camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <variant>

namespace
{

template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

} // namespace

auto Camera::getViewMatrix(
    const glm::vec3 &translation,
    const glm::quat &rotation) const -> glm::mat4
{
    return glm::inverse(
        glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation));
}

auto Camera::getProjectionMatrix(float viewportAspect) const -> glm::mat4
{
    const auto visitor = overloads{
        [&](const PerspectiveCamera &p) -> glm::mat4 {
            const float a = p.aspectRatio.has_value() ? *p.aspectRatio : viewportAspect;

            glm::mat4 proj{1.0f};

            if (p.zfar.has_value()) {
                proj = glm::perspectiveRH_ZO(p.yfov, a, p.znear, *p.zfar);
            } else {
                proj = glm::infinitePerspectiveRH_ZO(p.yfov, a, p.znear);
            }

            proj[1][1] *= -1.0f;
            return proj;
        },
        [&](const OrthographicCamera &o) -> glm::mat4 {
            const float l = -o.xmag;
            const float r = o.xmag;
            const float b = -o.ymag;
            const float t = o.ymag;

            glm::mat4 proj = glm::orthoRH_ZO(l, r, b, t, o.znear, o.zfar);

            proj[1][1] *= -1.0f;
            return proj;
        },
    };

    return std::visit(visitor, model);
}
