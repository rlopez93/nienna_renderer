#pragma once

#include <cstdint>
#include <glm/vec4.hpp>

struct DrawItem {
    uint32_t indexCount   = 0u;
    uint32_t firstIndex   = 0u;
    int32_t  vertexOffset = 0;

    uint32_t meshIndex    = 0u;
    uint32_t submeshIndex = 0u;

    uint32_t geometryIndex = 0u;

    uint32_t nodeInstanceIndex = 0u;
    uint32_t materialIndex     = 0u;
};
