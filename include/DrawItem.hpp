#pragma once

#include <cstdint>
#include <glm/vec4.hpp>

struct DrawItem {
    // Geometry
    uint32_t indexCount   = 0u; // vkCmdDrawIndexed indexCount
    uint32_t firstIndex   = 0u; // usually 0 for now
    int32_t  vertexOffset = 0;  // usually 0 for now

    // glTF
    uint32_t meshIndex      = 0u;
    uint32_t primitiveIndex = 0u;

    // GPU slot
    uint32_t geometryIndex = 0u;

    // instance/material
    uint32_t objectIndex   = 0u;
    uint32_t materialIndex = 0u;
};
