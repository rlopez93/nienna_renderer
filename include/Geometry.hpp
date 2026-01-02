#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};

    glm::vec4 tangent{0.0f, 0.0f, 0.0f, 1.0f};

    glm::vec2 uv0{0.0f};
    glm::vec2 uv1{0.0f};

    glm::vec4 color{1.0f};
};

struct Primitive {
    std::vector<Vertex>        vertices;
    std::vector<std::uint16_t> indices;

    std::uint32_t materialIndex = 0u;

    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;

    bool tangentsValid = false;
};

struct Mesh {
    std::vector<Primitive> primitives;
};
