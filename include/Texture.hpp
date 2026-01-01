#pragma once

#include <cstddef>
#include <vector>

#include <vulkan/vulkan.hpp>

struct Texture {
    std::vector<std::byte> rgba8;

    vk::Extent2D extent{0u, 0u};

    vk::SamplerCreateInfo samplerInfo{};

    bool isValid() const
    {
        if (extent.width == 0u || extent.height == 0u) {
            return false;
        }

        const std::size_t w = static_cast<std::size_t>(extent.width);

        const std::size_t h = static_cast<std::size_t>(extent.height);

        const std::size_t expectedBytes = w * h * 4u;

        return rgba8.size() == expectedBytes;
    }
};
