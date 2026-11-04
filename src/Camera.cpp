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
    glm::vec3 const& translation,
    glm::quat const& rotation) const -> glm::mat4
{
    return glm::inverse(
        glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation));
}

auto Camera::getProjectionMatrix(float viewportAspect) const -> glm::mat4
{
    auto const visitor = overloads{
        [&](PerspectiveCamera const& p) -> glm::mat4 {
            auto const a = p.aspectRatio.has_value() ? *p.aspectRatio : viewportAspect;

            auto proj = glm::mat4{1.0f};

            if (p.zfar.has_value()) {
                proj = glm::perspectiveRH_ZO(p.yfov, a, p.znear, *p.zfar);
            } else {
                proj = glm::infinitePerspectiveRH_ZO(p.yfov, a, p.znear);
            }

            proj[1][1] *= -1.0f;
            return proj;
        },
        [&](OrthographicCamera const& o) -> glm::mat4 {
            auto const l = -o.xmag;
            auto const r = o.xmag;
            auto const b = -o.ymag;
            auto const t = o.ymag;

            auto proj = glm::orthoRH_ZO(l, r, b, t, o.znear, o.zfar);

            proj[1][1] *= -1.0f;
            return proj;
        },
    };

    return std::visit(visitor, model);
}
