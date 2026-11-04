#pragma once

#include <cstddef>
#include <vector>

#include <vulkan/vulkan.hpp>

struct Texture {
    std::vector<std::byte> rgba8;

    vk::Extent2D extent{0u, 0u};

    vk::SamplerCreateInfo samplerInfo{};

    auto isValid() const -> bool
    {
        if (extent.width == 0u || extent.height == 0u) {
            return false;
        }

        auto const w = static_cast<std::size_t>(extent.width);

        auto const h = static_cast<std::size_t>(extent.height);

        auto const expectedBytes = static_cast<std::size_t>(w * h * 4u);

        return rgba8.size() == expectedBytes;
    }
};
