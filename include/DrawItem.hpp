#pragma once

#include <cstdint>
#include <glm/vec4.hpp>

struct DrawItem {
    // Geometry
    uint32_t indexCount;   // vkCmdDrawIndexed indexCount
    uint32_t firstIndex;   // usually 0 for now
    int32_t  vertexOffset; // usually 0 for now

    // Resource indices
    uint32_t transformIndex; // index into per-frame transform UBO
    int32_t  textureIndex;   // index into texture array descriptor (-1 = none)

    // Material overrides
    glm::vec4 baseColor; // push constant
};
